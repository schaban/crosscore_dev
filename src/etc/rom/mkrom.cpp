#include <string>
#include <vector>
#include <iostream>

#include "crosscore.hpp"

#define XROM_SIG XD_FOURCC('X', 'R', 'O', 'M')

static void dbgmsg(const char* pMsg) {
	::printf("%s", pMsg);
}

static void init_sys() {
	sxSysIfc sysIfc;
	nxCore::mem_zero(&sysIfc, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg;
	nxSys::init(&sysIfc);
}

struct FileInfo {
	struct Path {
		uint32_t tag; // len, hash
		char str[60];

		void reset() {
			nxCore::mem_zero(this, sizeof(FileInfo));
		}

		void set(const char* pStr) {
			reset();
			if (pStr) {
				size_t len = nxCore::str_len(pStr);
				if (len <= sizeof(str)) {
					tag = uint32_t((uint16_t)len) | (uint32_t(nxCore::str_hash16(pStr)) << 16);
					nxCore::mem_copy(str, pStr, len);
				}
			}
		}
	};
	Path path;
	uint32_t offs;
	uint32_t size;
	uint32_t reserved;
	uint32_t reserved2;
};

struct ROMHead {
	uint32_t sig;
	uint32_t size;
	uint32_t hsize;
	uint32_t nfiles;
};

struct SrcFile {
	std::string path;
	sxPackedData* pPk;
	void* pRaw;
	size_t size;
};

int mkrom() {
	using namespace std;

	bool dataRaw = nxApp::get_bool_opt("raw", false);

	vector<string> flst;
	for (string fpath; getline(cin, fpath);) {
		flst.push_back(fpath);
	}
	uint32_t nfiles = (uint32_t)flst.size();
	cout << "num files: " << flst.size() << endl;
	if (nfiles < 1) return -1;

	size_t headSize = sizeof(ROMHead) + (nfiles * sizeof(FileInfo));
	size_t romSize = headSize;
	ROMHead* pHead = (ROMHead*)nxCore::mem_alloc(headSize, "ROM_head");
	if (!pHead) return -2;
	nxCore::mem_zero(pHead, headSize);
	pHead->sig = XROM_SIG;
	pHead->hsize = uint32_t(headSize);
	pHead->nfiles = uint32_t(nfiles);
	FileInfo* pInfoTop = (FileInfo*)(pHead + 1);
	FileInfo* pInfo = pInfoTop;
	for (auto fpath : flst) {
		const char* pPathSrc = fpath.c_str();
		size_t pathLen = nxCore::str_len(pPathSrc);
		pInfo->path.reset();
		if (pathLen > 60) {
			cout << "! path too long: " << fpath << endl;
		}
		pInfo->path.set(pPathSrc);
		++pInfo;
	}

	vector<SrcFile> srcFiles;
	size_t totalSizeRaw = 0;
	size_t totalSize = 0;
	for (auto fpath : flst) {
		SrcFile src;
		src.path = fpath;
		size_t rawSize = 0;
		uint8_t* pRaw = (uint8_t*)nxCore::bin_load(fpath.c_str(), &rawSize);
		totalSizeRaw += rawSize;
		cout << "+ " << fpath << ", " << rawSize << " bytes: ";
		sxPackedData* pPk = nullptr;
		if (!dataRaw) {
			pPk = nxData::pack(pRaw, rawSize, 2);
			if (!pPk) {
				pPk = nxData::pack(pRaw, rawSize, 0);
			}
		}
		if (pPk) {
			cout << "compressed to " << pPk->mPackSize << " bytes";
			src.pPk = pPk;
			src.pRaw = nullptr;
			src.size = pPk->mPackSize;
			nxCore::bin_unload(pRaw);
		} else {
			if (dataRaw) {
				cout << "storing uncompressed";
			} else {
				cout << "can't compress";
			}
			src.pPk = nullptr;
			src.pRaw = pRaw;
			src.size = rawSize;
		}
		totalSize += src.size;
		srcFiles.push_back(src);
		cout << endl;
	}
	romSize += totalSize;

	cout << "total size (raw): " << totalSizeRaw << " bytes" << endl;
	cout << "      total size: " << totalSize << " bytes" << endl;

	pHead->size = uint32_t(romSize);
	pInfo = pInfoTop;
	uint32_t offs = uint32_t(headSize);
	for (auto src : srcFiles) {
		pInfo->offs = offs;
		pInfo->size = uint32_t(src.size);
		offs += pInfo->size;
		++pInfo;
	}

	size_t romAllocSize = nxCore::align_pad(romSize + 1, 0x10);
	void* pROM = nxCore::mem_alloc(romAllocSize, "ROM");
	if (!pROM) return -3;
	nxCore::mem_zero(pROM, romAllocSize);

	nxCore::mem_copy(pROM, pHead, headSize);
	void* pDataDst = XD_INCR_PTR(pROM, headSize);
	for (auto src : srcFiles) {
		nxCore::mem_copy(pDataDst, src.pPk ? src.pPk : src.pRaw, src.size);
		pDataDst = XD_INCR_PTR(pDataDst, src.size);
	}

	const char* pOutPath = nxApp::get_opt("o");
	if (!pOutPath) {
		if (nxApp::get_bool_opt("nobin", false)) {
			pOutPath = nullptr;
		} else {
			pOutPath = "xrom";
		}
	}
	if (pOutPath) {
		nxCore::bin_save(pOutPath, pROM, romAllocSize);
	}

	const char* pTextOutPath = nxApp::get_opt("txt");
	if (pTextOutPath) {
		FILE* pOut = nxSys::fopen_w_txt(pTextOutPath);
		if (pOut) {
			::fprintf(pOut, "extern \"C\" const unsigned char _binary_xrom_start[] = {");
			for (size_t i = 0; i < romAllocSize; ++i) {
				if ((i % 16) == 0) {
					::fprintf(pOut, "\n");
				}
				::fprintf(pOut, "0x%02X,", ((uint8_t*)pROM)[i]);
			}
			::fprintf(pOut, "\n0, 0, 0, 0};\n");
			::fclose(pOut);
		}
	}

	return 0;
}


int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	bool res = 0;
	res = mkrom();

	nxApp::reset();
//	nxCore::mem_dbg();
	return res;
}
