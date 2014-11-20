#include "ncch.h"
#include "romfs.h"
#include <openssl/sha.h>

const u32 CNcch::s_uSignature = CONVERT_ENDIAN('NCCH');
const int CNcch::s_nBlockSize = 0x1000;

CNcch::CNcch()
	: m_pFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pRomFsXorFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pAccessControlExtendedFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_bVerbose(false)
	, m_fpNcch(nullptr)
	, m_nMediaUnitSize(1 << 9)
	, m_nExtendedHeaderOffset(0)
	, m_nExtendedHeaderSize(0)
	, m_nAccessControlExtendedOffset(0)
	, m_nAccessControlExtendedSize(0)
	, m_nPlainRegionOffset(0)
	, m_nPlainRegionSize(0)
	, m_nExeFsOffset(0)
	, m_nExeFsSize(0)
	, m_nRomFsOffset(0)
	, m_nRomFsSize(0)
{
	memset(&m_NcchHeader, 0, sizeof(m_NcchHeader));
}

CNcch::~CNcch()
{
}

void CNcch::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CNcch::SetExtendedHeaderXorFileName(const char* a_pExtendedHeaderXorFileName)
{
	m_pExtendedHeaderXorFileName = a_pExtendedHeaderXorFileName;
}

void CNcch::SetExeFsXorFileName(const char* a_pExeFsXorFileName)
{
	m_pExeFsXorFileName = a_pExeFsXorFileName;
}

void CNcch::SetRomFsXorFileName(const char* a_pRomFsXorFileName)
{
	m_pRomFsXorFileName = a_pRomFsXorFileName;
}

void CNcch::SetHeaderFileName(const char* a_pHeaderFileName)
{
	m_pHeaderFileName = a_pHeaderFileName;
}

void CNcch::SetExtendedHeaderFileName(const char* a_pExtendedHeaderFileName)
{
	m_pExtendedHeaderFileName = a_pExtendedHeaderFileName;
}

void CNcch::SetAccessControlExtendedFileName(const char* a_pAccessControlExtendedFileName)
{
	m_pAccessControlExtendedFileName = a_pAccessControlExtendedFileName;
}

void CNcch::SetPlainRegionFileName(const char* a_pPlainRegionFileName)
{
	m_pPlainRegionFileName = a_pPlainRegionFileName;
}

void CNcch::SetExeFsFileName(const char* a_pExeFsFileName)
{
	m_pExeFsFileName = a_pExeFsFileName;
}

void CNcch::SetRomFsFileName(const char* a_pRomFsFileName)
{
	m_pRomFsFileName = a_pRomFsFileName;
}

void CNcch::SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash)
{
	m_bNotUpdateExtendedHeaderHash = a_bNotUpdateExtendedHeaderHash;
}

void CNcch::SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash)
{
	m_bNotUpdateExeFsHash = a_bNotUpdateExeFsHash;
}

void CNcch::SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash)
{
	m_bNotUpdateRomFsHash = a_bNotUpdateRomFsHash;
}

void CNcch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CNcch::CryptoFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	if (!cryptoFile(m_pExtendedHeaderXorFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, 0, "extendedheader"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pExtendedHeaderXorFileName, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize, m_nExtendedHeaderSize, "accesscontrolextended"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pExeFsXorFileName, m_nExeFsOffset, m_nExeFsSize, 0, "exefs"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pRomFsXorFileName, m_nRomFsOffset, m_nRomFsSize, 0, "romfs"))
	{
		bResult = false;
	}
	return true;
}

bool CNcch::ExtractFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	calculateMediaUnitSize();
	calculateOffsetSize();
	if (!extractFile(m_pHeaderFileName, 0, sizeof(m_NcchHeader), "ncch header"))
	{
		bResult = false;
	}
	if (!extractFile(m_pExtendedHeaderFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, "extendedheader"))
	{
		bResult = false;
	}
	if (!extractFile(m_pAccessControlExtendedFileName, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize, "accesscontrolextended"))
	{
		bResult = false;
	}
	if (!extractFile(m_pPlainRegionFileName, m_nPlainRegionOffset, m_nPlainRegionSize, "plainregion"))
	{
		bResult = false;
	}
	if (!extractFile(m_pExeFsFileName, m_nExeFsOffset, m_nExeFsSize, "exefs"))
	{
		bResult = false;
	}
	if (!extractFile(m_pRomFsFileName, m_nRomFsOffset, m_nRomFsSize, "romfs"))
	{
		bResult = false;
	}
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::CreateFile()
{
	bool bResult = true;
	m_fpNcch = FFopen(m_pFileName, "wb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	if (!createHeader())
	{
		return false;
	}
	calculateMediaUnitSize();
	if (!createExtendedHeader())
	{
		fclose(m_fpNcch);
		bResult = false;
	}
	if (!createAccessControlExtended())
	{
		bResult = false;
	}
	alignFileSize(m_nMediaUnitSize);
	if (!createPlainRegion())
	{
		bResult = false;
	}
	if (!createExeFs())
	{
		bResult = false;
	}
	if (!createRomFs())
	{
		bResult = false;
	}
	FFseek(m_fpNcch, 0, SEEK_SET);
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::IsNcchFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SNcchHeader ncchHeader;
	fread(&ncchHeader, sizeof(ncchHeader), 1, fp);
	fclose(fp);
	return ncchHeader.Ncch.Signature == s_uSignature;
}

void CNcch::calculateMediaUnitSize()
{
	m_nMediaUnitSize = 1LL << (m_NcchHeader.Ncch.Flags[6] + 9);
}

void CNcch::calculateOffsetSize()
{
	m_nExtendedHeaderOffset = sizeof(m_NcchHeader);
	m_nExtendedHeaderSize = m_NcchHeader.Ncch.ExtendedHeaderSize;
	m_nPlainRegionOffset = m_NcchHeader.Ncch.PlainRegionOffset * m_nMediaUnitSize;
	if (m_nPlainRegionOffset < m_nExtendedHeaderOffset)
	{
		m_nPlainRegionOffset = m_nExtendedHeaderOffset + m_nExtendedHeaderSize;
	}
	m_nPlainRegionSize = m_NcchHeader.Ncch.PlainRegionSize * m_nMediaUnitSize;
	m_nAccessControlExtendedOffset = m_nExtendedHeaderOffset + m_nExtendedHeaderSize;
	m_nAccessControlExtendedSize = m_nPlainRegionOffset - m_nAccessControlExtendedOffset;
	m_nExeFsOffset = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
	m_nExeFsSize = m_NcchHeader.Ncch.ExeFsSize * m_nMediaUnitSize;
	m_nRomFsOffset = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
	m_nRomFsSize = m_NcchHeader.Ncch.RomFsSize * m_nMediaUnitSize;
}

bool CNcch::cryptoFile(const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType)
{
	bool bResult = true;
	if (a_pXorFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			bResult = FCryptoFile(m_pFileName, a_pXorFileName, a_nOffset, a_nSize, false, a_nXorOffset, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists\n", a_pType);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not decrypt or encrypt\n", a_pType);
	}
	return bResult;
}

bool CNcch::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			FILE* fp = FFopen(a_pFileName, "wb");
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					printf("save: %s\n", a_pFileName);
				}
				FCopyFile(fp, m_fpNcch, a_nOffset, a_nSize);
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists, %s will not be create\n", a_pType, a_pFileName);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not extract\n", a_pType);
	}
	return bResult;
}

bool CNcch::createHeader()
{
	FILE* fp = FFopen(m_pHeaderFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	FFseek(fp, 0, SEEK_END);
	n64 nFileSize = FFtell(fp);
	if (nFileSize < sizeof(m_NcchHeader))
	{
		fclose(fp);
		printf("ERROR: ncch header is too short\n\n");
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", m_pHeaderFileName);
	}
	FFseek(fp, 0, SEEK_SET);
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	fclose(fp);
	fwrite(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	return true;
}

bool CNcch::createExtendedHeader()
{
	if (m_pExtendedHeaderFileName != nullptr)
	{
		FILE* fp = FFopen(m_pExtendedHeaderFileName, "rb");
		if (fp == nullptr)
		{
			clearExtendedHeader();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExtendedHeaderFileName);
		}
		FFseek(fp, 0, SEEK_END);
		m_NcchHeader.Ncch.ExtendedHeaderSize = static_cast<u32>(FFtell(fp));
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[m_NcchHeader.Ncch.ExtendedHeaderSize];
		fread(pBuffer, 1, m_NcchHeader.Ncch.ExtendedHeaderSize, fp);
		fclose(fp);
		if (!m_bNotUpdateExtendedHeaderHash)
		{
			SHA256(pBuffer, m_NcchHeader.Ncch.ExtendedHeaderSize, m_NcchHeader.Ncch.ExtendedHeaderHash);
		}
		fwrite(pBuffer, 1, m_NcchHeader.Ncch.ExtendedHeaderSize, m_fpNcch);
		delete[] pBuffer;
	}
	else
	{
		clearExtendedHeader();
	}
	return true;
}

bool CNcch::createAccessControlExtended()
{
	if (m_pAccessControlExtendedFileName != nullptr)
	{
		FILE* fp = FFopen(m_pAccessControlExtendedFileName, "rb");
		if (fp == nullptr)
		{
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pAccessControlExtendedFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<unsigned int>(nFileSize)];
		fread(pBuffer, 1, static_cast<size_t>(nFileSize), fp);
		fclose(fp);
		fwrite(pBuffer, 1, static_cast<size_t>(nFileSize), m_fpNcch);
		delete[] pBuffer;
	}
	return true;
}

bool CNcch::createPlainRegion()
{
	if (m_pPlainRegionFileName != nullptr)
	{
		FILE* fp = FFopen(m_pPlainRegionFileName, "rb");
		if (fp == nullptr)
		{
			clearPlainRegion();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pPlainRegionFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		m_NcchHeader.Ncch.PlainRegionOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.PlainRegionSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<unsigned int>(nFileSize)];
		fread(pBuffer, 1, static_cast<size_t>(nFileSize), fp);
		fclose(fp);
		fwrite(pBuffer, 1, static_cast<size_t>(nFileSize), m_fpNcch);
		delete[] pBuffer;
	}
	else
	{
		clearPlainRegion();
	}
	alignFileSize(m_nMediaUnitSize);
	return true;
}

bool CNcch::createExeFs()
{
	if (m_pExeFsFileName != nullptr)
	{
		FILE* fp = FFopen(m_pExeFsFileName, "rb");
		if (fp == nullptr)
		{
			clearExeFs();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pExeFsFileName);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		if (nFileSize < 512)
		{
			fclose(fp);
			clearExeFs();
			printf("ERROR: exefs is too short\n\n");
			return false;
		}
		n64 nSuperBlockSize = FAlign(512, m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearExeFs();
			printf("ERROR: exefs is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.ExeFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.ExeFsSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (!m_bNotUpdateExeFsHash)
		{
			m_NcchHeader.Ncch.ExeFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		}
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<unsigned int>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		if (!m_bNotUpdateExeFsHash)
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.ExeFsSuperBlockHash);
		}
		fwrite(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), m_fpNcch);
		delete[] pBuffer;
		FCopyFile(m_fpNcch, fp, nSuperBlockSize, nFileSize - nSuperBlockSize);
		fclose(fp);
	}
	else
	{
		clearExeFs();
	}
	alignFileSize(s_nBlockSize);
	return true;
}

bool CNcch::createRomFs()
{
	if (m_pRomFsFileName != nullptr)
	{
		FILE* fp = FFopen(m_pRomFsFileName, "rb");
		if (fp == nullptr)
		{
			clearRomFs();
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pRomFsFileName);
		}
		SRomFsHeader romFsHeader;
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		if (nFileSize < sizeof(romFsHeader))
		{
			fclose(fp);
			clearRomFs();
			printf("ERROR: romfs is too short\n\n");
			return false;
		}
		FFseek(fp, 0, SEEK_SET);
		fread(&romFsHeader, sizeof(romFsHeader), 1, fp);
		n64 nSuperBlockSize = FAlign(FAlign(sizeof(romFsHeader), CRomFs::s_nSHA256BlockSize) + romFsHeader.Level0Size, m_nMediaUnitSize);
		if (nFileSize < nSuperBlockSize)
		{
			fclose(fp);
			clearRomFs();
			printf("ERROR: romfs is too short\n\n");
			return false;
		}
		m_NcchHeader.Ncch.RomFsOffset = m_NcchHeader.Ncch.ContentSize;
		m_NcchHeader.Ncch.RomFsSize = static_cast<u32>(FAlign(nFileSize, m_nMediaUnitSize) / m_nMediaUnitSize);
		if (!m_bNotUpdateRomFsHash)
		{
			m_NcchHeader.Ncch.RomFsHashRegionSize = static_cast<u32>(nSuperBlockSize / m_nMediaUnitSize);
		}
		FFseek(fp, 0, SEEK_SET);
		u8* pBuffer = new u8[static_cast<unsigned int>(nSuperBlockSize)];
		fread(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), fp);
		if (!m_bNotUpdateRomFsHash)
		{
			SHA256(pBuffer, static_cast<size_t>(nSuperBlockSize), m_NcchHeader.Ncch.RomFsSuperBlockHash);
		}
		fwrite(pBuffer, 1, static_cast<size_t>(nSuperBlockSize), m_fpNcch);
		delete[] pBuffer;
		FCopyFile(m_fpNcch, fp, nSuperBlockSize, nFileSize - nSuperBlockSize);
		fclose(fp);
	}
	else
	{
		clearRomFs();
	}
	alignFileSize(s_nBlockSize);
	return true;
}

void CNcch::clearExtendedHeader()
{
	memset(m_NcchHeader.Ncch.ExtendedHeaderHash, 0, sizeof(m_NcchHeader.Ncch.ExtendedHeaderHash));
	m_NcchHeader.Ncch.ExtendedHeaderSize = 0;
}

void CNcch::clearPlainRegion()
{
	m_NcchHeader.Ncch.PlainRegionOffset = 0;
	m_NcchHeader.Ncch.PlainRegionSize = 0;
}

void CNcch::clearExeFs()
{
	m_NcchHeader.Ncch.ExeFsOffset = 0;
	m_NcchHeader.Ncch.ExeFsSize = 0;
	m_NcchHeader.Ncch.ExeFsHashRegionSize = 0;
	memset(m_NcchHeader.Ncch.ExeFsSuperBlockHash, 0, sizeof(m_NcchHeader.Ncch.ExeFsSuperBlockHash));
}

void CNcch::clearRomFs()
{
	m_NcchHeader.Ncch.RomFsOffset = 0;
	m_NcchHeader.Ncch.RomFsSize = 0;
	m_NcchHeader.Ncch.RomFsHashRegionSize = 0;
	memset(m_NcchHeader.Ncch.RomFsSuperBlockHash, 0, sizeof(m_NcchHeader.Ncch.RomFsSuperBlockHash));
}

void CNcch::alignFileSize(n64 a_nAlignment)
{
	FFseek(m_fpNcch, 0, SEEK_END);
	n64 nFileSize = FAlign(FFtell(m_fpNcch), a_nAlignment);
	FSeek(m_fpNcch, nFileSize);
	m_NcchHeader.Ncch.ContentSize = static_cast<u32>(nFileSize / m_nMediaUnitSize);
}
