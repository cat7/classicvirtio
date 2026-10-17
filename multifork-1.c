/* Copyright (c) 2024 Elliot Nunn */
/* Licensed under the MIT license */

/*
Implementation of the xattr-based multifork format
(best for interoperability with host tools)

Data fork     = the file itself
Resource fork = com.apple.ResourceFork xattr (binary, raw resource fork)
Finder info   = com.apple.FinderInfo xattr (32 bytes, binary)

No sidecar files. Works on any host filesystem with xattr support.
Requires QEMU 9P xattr namespace patch for com.apple.* passthrough.
*/

#include <string.h>

#include "9buf.h"
#include "9p.h"
#include "catalog.h"
#include "fids.h"
#include "panic.h"
#include "printf.h"
#include "universalfcb.h"

#include "multifork.h"

#include <stdint.h>
#include <stdlib.h>

// Fids 8-15 are reserved for the multifork layer
enum {
	XFID = FIRSTFID_MULTIFORK, // scratch fid for xattr walk source
	XFID2,                     // scratch fid for xattr walk result
	RFORKFID,                  // resource fork open file
	PARENTFID1,                // scratch for walking to parent
	TMPFID1,                   // scratch for delete operations
	LISTFID1,                  // scratch for listing a folder being deleted
	DIRTYFLAG = 1,
};

#define XATTR_FINDERINFO   "com.apple.FinderInfo"
#define XATTR_RESOURCEFORK "com.apple.ResourceFork"
#define FINDERINFO_SIZE    32

static uint32_t fidof(struct MyFCB *fcb) {
	return 32UL + fcb->refNum;
}

// Read an xattr into buf. Returns bytes read, or 0 on any error.
static uint32_t read_xattr(uint32_t fid, const char *xname, void *buf, uint32_t bufsz) {
	uint64_t size = 0;

	// Walk to file first, to get a disposable fid
	if (WalkPath9(fid, XFID, ""))
		return 0;

	// Xattrwalk needs distinct fid and newfid (QEMU requirement)
	if (Xattrwalk9(XFID, XFID2, xname, &size)) {
		Clunk9(XFID);
		return 0;
	}
	Clunk9(XFID); // done with the file fid

	if (size == 0 || size > bufsz) {
		Clunk9(XFID2);
		return 0;
	}

	uint32_t actual = 0;
	if (Read9(XFID2, buf, 0, (uint32_t)size, &actual)) {
		Clunk9(XFID2);
		return 0;
	}

	Clunk9(XFID2);
	return actual;
}

// Write an xattr. Returns 0 on success.
static int write_xattr(uint32_t fid, const char *xname, const void *buf, uint32_t size) {
	// Walk to get a disposable fid (Xattrcreate consumes the fid)
	if (WalkPath9(fid, XFID, ""))
		return EIO;

	if (Xattrcreate9(XFID, xname, size, 0)) {
		Clunk9(XFID);
		return EIO;
	}

	// Write the data, then clunk to commit
	if (size > 0) {
		uint32_t actual = 0;
		int err = Write9(XFID, buf, 0, size, &actual);
		if (err || actual != size) {
			Clunk9(XFID);
			return EIO;
		}
	}

	Clunk9(XFID);
	return 0;
}

static int init1(void) {
	// No initialization needed -- no hidden directories or caches
	return 0;
}

static int open1(struct MyFCB *fcb, int32_t cnid, uint32_t fid, const char *name) {
	uint32_t myfid = fidof(fcb);

	if (fcb->fcbFlags & fcbResourceMask) {
		// Resource fork: read xattr into a tempfile in .classicvirtio cache
		// We need a seekable file backing, so create one in the dotdir
		char path[20];
		sprintf(path, "rf-%08lx", (long)cnid);

		// Create (or truncate) the cache file
		WalkPath9(DOTDIRFID, myfid, "");
		int err = Lcreate9(myfid, O_RDWR | O_CREAT | O_TRUNC, 0666, 0, path, NULL, NULL);
		if (err) return err;

		// Read the resource fork xattr from the actual file
		// We need the parent fid + name to locate the file
		WalkPath9(fid, PARENTFID1, "..");

		// Walk to the file to read its xattr
		char rfbuf[32768]; // 32K buffer for resource fork transfer

		if (!WalkPath9(fid, XFID, "")) {
			uint64_t xsize = 0;
			if (!Xattrwalk9(XFID, XFID2, XATTR_RESOURCEFORK, &xsize) && xsize > 0) {
				// Read in chunks and write to cache file
				uint64_t offset = 0;
				while (offset < xsize) {
					uint32_t chunk = sizeof rfbuf;
					if (offset + chunk > xsize) chunk = (uint32_t)(xsize - offset);
					uint32_t actual = 0;
					if (Read9(XFID2, rfbuf, offset, chunk, &actual) || actual == 0) break;
					Write9(myfid, rfbuf, offset, actual, NULL);
					offset += actual;
				}
			}
			Clunk9(XFID2);
			Clunk9(XFID);
		}

		// Reopen the cache file for read/write
		Clunk9(myfid);
		WalkPath9(DOTDIRFID, myfid, path);

		if (fcb->fcbFlags & fcbWriteMask) {
			err = Lopen9(myfid, O_RDWR, NULL, NULL);
			if (err == 0) return 0;
		}
		return Lopen9(myfid, O_RDONLY, NULL, NULL);

	} else {
		// Data fork: just open the file directly
		WalkPath9(fid, myfid, "");

		if (fcb->fcbFlags & fcbWriteMask) {
			int err = Lopen9(myfid, O_RDWR, NULL, NULL);
			if (err == 0) return 0;
		}
		return Lopen9(myfid, O_RDONLY, NULL, NULL);
	}
}

// Flush resource fork cache back to xattr
static void flush_rfork(struct MyFCB *fcb) {
	uint32_t myfid = fidof(fcb);
	char name[MAXNAME];
	int32_t parent = CatalogGet(fcb->fcbFlNm, name);
	if (IsErr(parent)) return;
	if (IsErr(CatalogWalk(PARENTFID1, parent, NULL, NULL, NULL))) return;

	// Stat the cache file to get size
	struct Stat9 cstat = {};
	Getattr9(myfid, STAT_SIZE, &cstat);

	if (cstat.size == 0) {
		// Empty resource fork: delete the xattr if it exists
		// (best-effort, ignore errors)
		if (!WalkPath9(PARENTFID1, XFID, name)) {
			Xattrcreate9(XFID, XATTR_RESOURCEFORK, 0, 0);
			Clunk9(XFID);
		}
		return;
	}

	// Walk to the actual file for xattr writing
	if (WalkPath9(PARENTFID1, XFID, name)) return;

	if (Xattrcreate9(XFID, XATTR_RESOURCEFORK, cstat.size, 0)) {
		Clunk9(XFID);
		return;
	}

	// Copy from cache file to xattr
	char buf[32768];
	uint64_t offset = 0;
	while (offset < cstat.size) {
		uint32_t chunk = sizeof buf;
		if (offset + chunk > cstat.size) chunk = (uint32_t)(cstat.size - offset);
		uint32_t actual = 0;
		if (Read9(myfid, buf, offset, chunk, &actual) || actual == 0) break;
		Write9(XFID, buf, offset, actual, NULL);
		offset += actual;
	}

	Clunk9(XFID);
}

static int close1(struct MyFCB *fcb) {
	if (fcb->fcbFlags & fcbResourceMask) {
		// fcb is already delisted, so any FCB found is another opener
		bool last = UnivFirst(fcb->fcbFlNm, true) == NULL;

		if (fcb->mfFlags & DIRTYFLAG) {
			// Clear dirty on all FCBs for this file
			for (struct MyFCB *i = UnivFirst(fcb->fcbFlNm, true); i != NULL; i = UnivNext(i)) {
				i->mfFlags &= ~DIRTYFLAG;
			}
			flush_rfork(fcb);
		}

		// The cache file is only needed while the fork is open
		if (last) {
			char path[20];
			sprintf(path, "rf-%08lx", (long)fcb->fcbFlNm);
			Unlinkat9(DOTDIRFID, path, 0);
		}
	}

	return Clunk9(fidof(fcb));
}

static int read1(struct MyFCB *fcb, void *buf, uint64_t offset, uint32_t count, uint32_t *actual_count) {
	return Read9(fidof(fcb), buf, offset, count, actual_count);
}

static int write1(struct MyFCB *fcb, const void *buf, uint64_t offset, uint32_t count, uint32_t *actual_count) {
	if ((fcb->fcbFlags & fcbResourceMask) && !(fcb->mfFlags & DIRTYFLAG)) {
		for (struct MyFCB *i = UnivFirst(fcb->fcbFlNm, true); i != NULL; i = UnivNext(fcb)) {
			i->mfFlags |= DIRTYFLAG;
		}
	}
	return Write9(fidof(fcb), buf, offset, count, actual_count);
}

static int geteof1(struct MyFCB *fcb, uint64_t *len) {
	struct Stat9 stat = {};
	int err = Getattr9(fidof(fcb), STAT_SIZE, &stat);
	if (err) return err;
	*len = stat.size;
	return 0;
}

static int seteof1(struct MyFCB *fcb, uint64_t len) {
	int err = Setattr9(fidof(fcb), SET_SIZE, (struct Stat9){.size = len});
	if (err) return err;

	// Flush resource fork on truncate/resize
	if ((fcb->fcbFlags & fcbResourceMask) && ((fcb->mfFlags & DIRTYFLAG) || len == 0)) {
		for (struct MyFCB *i = UnivFirst(fcb->fcbFlNm, true); i != NULL; i = UnivNext(fcb)) {
			i->mfFlags &= ~DIRTYFLAG;
		}
		flush_rfork(fcb);
	}
	return 0;
}

static int fgetattr1(int32_t cnid, uint32_t fid, const char *name, unsigned fields, struct MFAttr *attr) {
	memset(attr, 0, sizeof *attr);

	// Stat the data fork (essential)
	if ((fields & MF_DSIZE) || (fields & MF_TIME)) {
		struct Stat9 dstat = {};
		int err = Getattr9(fid,
			((fields & MF_DSIZE) ? STAT_SIZE : 0) |
			((fields & MF_TIME) ? STAT_MTIME : 0),
			&dstat);
		if (err) return err;

		attr->dsize = dstat.size;
		attr->unixtime = dstat.mtime_sec;
	}

	// Read resource fork size from xattr
	if (fields & MF_RSIZE) {
		uint64_t xsize = 0;
		if (!WalkPath9(fid, XFID, "")) {
			if (!Xattrwalk9(XFID, XFID2, XATTR_RESOURCEFORK, &xsize)) {
				attr->rsize = xsize;
				Clunk9(XFID2);
			}
			Clunk9(XFID);
		}
	}

	// Read FinderInfo from xattr
	if (fields & MF_FINFO) {
		char fi[FINDERINFO_SIZE];
		uint32_t got = read_xattr(fid, XATTR_FINDERINFO, fi, sizeof fi);
		if (got >= 16) {
			memcpy(attr->finfo, fi, 16);
		}
		if (got >= 32) {
			memcpy(attr->fxinfo, fi + 16, 16);
		}
	}

	return 0;
}

static int fsetattr1(int32_t cnid, uint32_t fid, const char *name, unsigned fields, const struct MFAttr *attr) {
	if (fields & MF_TIME) {
		// Could set mtime here, but MF3 doesn't either
	}

	if (fields & MF_FINFO) {
		// Compose the 32-byte FinderInfo blob
		char fi[FINDERINFO_SIZE];
		memset(fi, 0, sizeof fi);
		memcpy(fi, attr->finfo, 16);
		memcpy(fi + 16, attr->fxinfo, 16);

		write_xattr(fid, XATTR_FINDERINFO, fi, FINDERINFO_SIZE);
	}

	return 0;
}

static int dgetattr1(int32_t cnid, uint32_t fid, const char *name, unsigned fields, struct MFAttr *attr) {
	memset(attr, 0, sizeof *attr);

	if (fields & MF_TIME) {
		struct Stat9 dstat = {};
		int err = Getattr9(fid, STAT_MTIME, &dstat);
		if (err) return err;
		attr->unixtime = dstat.mtime_sec;
	}

	// Directory FinderInfo from xattr
	if (fields & MF_FINFO) {
		char fi[FINDERINFO_SIZE];
		uint32_t got = read_xattr(fid, XATTR_FINDERINFO, fi, sizeof fi);
		if (got >= 16) {
			memcpy(attr->finfo, fi, 16);
		}
		if (got >= 32) {
			memcpy(attr->fxinfo, fi + 16, 16);
		}
	}

	return 0;
}

static int dsetattr1(int32_t cnid, uint32_t fid, const char *name, unsigned fields, const struct MFAttr *attr) {
	if (fields & MF_FINFO) {
		char fi[FINDERINFO_SIZE];
		memset(fi, 0, sizeof fi);
		memcpy(fi, attr->finfo, 16);
		memcpy(fi + 16, attr->fxinfo, 16);

		write_xattr(fid, XATTR_FINDERINFO, fi, FINDERINFO_SIZE);
	}

	return 0;
}

static int move1(uint32_t fid1, const char *name1, uint32_t fid2, const char *name2) {
	// Xattrs travel with the file, nothing extra to do
	return Renameat9(fid1, name1, fid2, name2);
}

// Dot-names are hidden from the guest (e.g. .DS_Store), so a folder that
// holds them could never be emptied. Remove them from folder @dirfid.
// Hidden subfolders go only if empty. Returns the number removed.
static int purge_hidden1(uint32_t dirfid) {
	enum {BATCH = 16};
	char names[BATCH][MAXNAME];
	char types[BATCH];
	int n = 0, removed = 0;
	char buf[4096];
	uint64_t offset = 0;
	uint32_t count = 0;

	WalkPath9(dirfid, LISTFID1, "");
	if (Lopen9(LISTFID1, O_RDONLY|O_DIRECTORY, NULL, NULL)) {
		Clunk9(LISTFID1);
		return 0;
	}
	while (n < BATCH && !Readdir9(LISTFID1, offset, sizeof buf, &count, buf) && count > 0) {
		char *p = buf;
		while (n < BATCH && p < buf + count) {
			DirRecord9(&p, NULL, &offset, &types[n], names[n]);
			if (names[n][0] != '.' || !strcmp(names[n], ".") || !strcmp(names[n], "..")) continue;
			n++;
		}
	}
	Clunk9(LISTFID1);

	for (int i=0; i<n; i++) {
		if (!Unlinkat9(dirfid, names[i], types[i] == 4 ? 0x200 /*AT_REMOVEDIR*/ : 0)) {
			removed++;
		}
	}
	return removed;
}

static int del1(uint32_t fid, const char *name, bool isdir) {
	WalkPath9(fid, TMPFID1, "..");

	if (isdir) {
		int err = Unlinkat9(TMPFID1, name, 0x200 /*AT_REMOVEDIR*/);
		while (err == ENOTEMPTY && purge_hidden1(fid) > 0) {
			err = Unlinkat9(TMPFID1, name, 0x200 /*AT_REMOVEDIR*/);
		}
		return err;
	} else {
		// Xattrs are deleted automatically with the file
		return Unlinkat9(TMPFID1, name, 0);
	}
}

static bool issidecar1(const char *name) {
	// Hide system metadata folders that shouldn't appear in listings
	if (!strcmp(name, "TheVolumeSettingsFolder")) return true;
	return false;
}

struct MFImpl MF1 = {
	.Name = "xattr",
	.Init = &init1,
	.Open = &open1,
	.Close = &close1,
	.Read = &read1,
	.Write = &write1,
	.GetEOF = &geteof1,
	.SetEOF = &seteof1,
	.FGetAttr = &fgetattr1,
	.FSetAttr = &fsetattr1,
	.DGetAttr = &dgetattr1,
	.DSetAttr = &dsetattr1,
	.Move = &move1,
	.Del = &del1,
	.IsSidecar = &issidecar1,
};
