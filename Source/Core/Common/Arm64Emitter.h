// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "Common/ArmCommon.h"
#include "Common/BitSet.h"
#include "Common/CodeBlock.h"
#include "Common/Common.h"

namespace Arm64Gen
{

// X30 serves a dual purpose as a link register
// Encoded as <u3:type><u5:reg>
// Types:
// 000 - 32bit GPR
// 001 - 64bit GPR
// 010 - VFP single precision
// 100 - VFP double precision
// 110 - VFP quad precision
enum ARM64Reg
{
	// 32bit registers
	W0 = 0, W1, W2, W3, W4, W5, W6,
	W7, W8, W9, W10, W11, W12, W13, W14,
	W15, W16, W17, W18, W19, W20, W21, W22,
	W23, W24, W25, W26, W27, W28, W29, W30,

	WSP, // 32bit stack pointer

	// 64bit registers
	X0 = 0x20, X1, X2, X3, X4, X5, X6,
	X7, X8, X9, X10, X11, X12, X13, X14,
	X15, X16, X17, X18, X19, X20, X21, X22,
	X23, X24, X25, X26, X27, X28, X29, X30,

	SP, // 64bit stack pointer

	// VFP single precision registers
	S0 = 0x40, S1, S2, S3, S4, S5, S6,
	S7, S8, S9, S10, S11, S12, S13,
	S14, S15, S16, S17, S18, S19, S20,
	S21, S22, S23, S24, S25, S26, S27,
	S28, S29, S30, S31,

	// VFP Double Precision registers
	D0 = 0x80, D1, D2, D3, D4, D5, D6, D7,
	D8, D9, D10, D11, D12, D13, D14, D15,
	D16, D17, D18, D19, D20, D21, D22, D23,
	D24, D25, D26, D27, D28, D29, D30, D31,

	// ASIMD Quad-Word registers
	Q0 = 0xC0, Q1, Q2, Q3, Q4, Q5, Q6, Q7,
	Q8, Q9, Q10, Q11, Q12, Q13, Q14, Q15,
	Q16, Q17, Q18, Q19, Q20, Q21, Q22, Q23,
	Q24, Q25, Q26, Q27, Q28, Q29, Q30, Q31,

	// For PRFM(prefetch memory) encoding
	// This is encoded in the Rt register
	// Data preload
	PLDL1KEEP = 0, PLDL1STRM,
	PLDL2KEEP, PLDL2STRM,
	PLDL3KEEP, PLDL3STRM,
	// Instruction preload
	PLIL1KEEP = 8, PLIL1STRM,
	PLIL2KEEP, PLIL2STRM,
	PLIL3KEEP, PLIL3STRM,
	// Prepare for store
	PLTL1KEEP = 16, PLTL1STRM,
	PLTL2KEEP, PLTL2STRM,
	PLTL3KEEP, PLTL3STRM,

	INVALID_REG = 0xFFFFFFFF
};

inline bool Is64Bit(ARM64Reg reg) { return reg & 0x20; }
inline bool IsSingle(ARM64Reg reg) { return reg & 0x40; }
inline bool IsDouble(ARM64Reg reg) { return reg & 0x80; }
inline bool IsQuad(ARM64Reg reg) { return (reg & 0xC0) == 0xC0; }
inline bool IsVector(ARM64Reg reg) { return (reg & 0xC0) != 0; }
inline ARM64Reg DecodeReg(ARM64Reg reg) { return (ARM64Reg)(reg & 0x1F); }
inline ARM64Reg EncodeRegTo64(ARM64Reg reg) { return (ARM64Reg)(reg | 0x20); }
inline ARM64Reg EncodeRegToDouble(ARM64Reg reg) { return (ARM64Reg)((reg & ~0xC0) | 0x80); }
inline ARM64Reg EncodeRegToQuad(ARM64Reg reg) { return (ARM64Reg)(reg | 0xC0); }

enum OpType
{
	TYPE_IMM = 0,
	TYPE_REG,
	TYPE_IMMSREG,
	TYPE_RSR,
	TYPE_MEM
};

enum ShiftType
{
	ST_LSL = 0,
	ST_LSR = 1,
	ST_ASR = 2,
	ST_ROR = 3,
};

enum IndexType
{
	INDEX_UNSIGNED,
	INDEX_POST,
	INDEX_PRE,
};

enum ShiftAmount
{
	SHIFT_0 = 0,
	SHIFT_16 = 1,
	SHIFT_32 = 2,
	SHIFT_48 = 3,
};

enum ExtendType
{
	EXTEND_UXTW = 2,
	EXTEND_LSL = 3, // Default for zero shift amount
	EXTEND_SXTW = 6,
	EXTEND_SXTX = 7,
};

struct FixupBranch
{
	u8* ptr;
	// Type defines
	// 0 = CBZ (32bit)
	// 1 = CBNZ (32bit)
	// 2 = B (conditional)
	// 3 = TBZ
	// 4 = TBNZ
	// 5 = B (unconditional)
	// 6 = BL (unconditional)
	u32 type;

	// Used with B.cond
	CCFlags cond;

	// Used with TBZ/TBNZ
	u8 bit;

	// Used with Test/Compare and Branch
	ARM64Reg reg;
};

enum PStateField
{
	FIELD_SPSel = 0,
	FIELD_DAIFSet,
	FIELD_DAIFClr,
};

enum SystemHint
{
	HINT_NOP = 0,
	HINT_YIELD,
	HINT_WFE,
	HINT_WFI,
	HINT_SEV,
	HINT_SEVL,
};

enum BarrierType
{
	OSHLD = 1,
	OSHST = 2,
	OSH   = 3,
	NSHLD = 5,
	NSHST = 6,
	NSH   = 7,
	ISHLD = 9,
	ISHST = 10,
	ISH   = 11,
	LD    = 13,
	ST    = 14,
	SY    = 15,
};

class ArithOption
{
public:
	enum WidthSpecifier
	{
		WIDTH_DEFAULT,
		WIDTH_32BIT,
		WIDTH_64BIT,
	};

	enum ExtendSpecifier
	{
		EXTEND_UXTB = 0x0,
		EXTEND_UXTH = 0x1,
		EXTEND_UXTW = 0x2, /* Also LSL on 32bit width */
		EXTEND_UXTX = 0x3, /* Also LSL on 64bit width */
		EXTEND_SXTB = 0x4,
		EXTEND_SXTH = 0x5,
		EXTEND_SXTW = 0x6,
		EXTEND_SXTX = 0x7,
	};

	enum TypeSpecifier
	{
		TYPE_EXTENDEDREG,
		TYPE_IMM,
		TYPE_SHIFTEDREG,
	};

private:
	ARM64Reg        m_destReg;
	WidthSpecifier  m_width;
	ExtendSpecifier m_extend;
	TypeSpecifier   m_type;
	ShiftType       m_shifttype;
	u32             m_shift;

public:
	ArithOption(ARM64Reg Rd, bool index = false)
	{
		// Indexed registers are a certain feature of AARch64
		// On Loadstore instructions that use a register offset
		// We can have the register as an index
		// If we are indexing then the offset register will
		// be shifted to the left so we are indexing at intervals
		// of the size of what we are loading
		// 8-bit: Index does nothing
		// 16-bit: Index LSL 1
		// 32-bit: Index LSL 2
		// 64-bit: Index LSL 3
		if (index)
			m_shift = 4;
		else
			m_shift = 0;

		m_destReg = Rd;
		m_type = TYPE_EXTENDEDREG;
		if (Is64Bit(Rd))
		{
			m_width = WIDTH_64BIT;
			m_extend = EXTEND_UXTX;
		}
		else
		{
			m_width = WIDTH_32BIT;
			m_extend = EXTEND_UXTW;
		}
	}
	ArithOption(ARM64Reg Rd, ShiftType shift_type, u32 shift)
	{
		m_destReg = Rd;
		m_shift = shift;
		m_shifttype = shift_type;
		m_type = TYPE_SHIFTEDREG;
		if (Is64Bit(Rd))
		{
			m_width = WIDTH_64BIT;
			if (shift == 64)
				m_shift = 0;
		}
		else
		{
			m_width = WIDTH_32BIT;
			if (shift == 32)
				m_shift = 0;
		}
	}
	TypeSpecifier GetType() const
	{
		return m_type;
	}
	ARM64Reg GetReg()
	{
		return m_destReg;
	}
	u32 GetData() const
	{
		switch (m_type)
		{
			case TYPE_EXTENDEDREG:
				return (m_extend << 13) |
				       (m_shift << 10);
			break;
			case TYPE_SHIFTEDREG:
				return (m_shifttype << 22) |
				       (m_shift << 10);
			break;
			default:
				_dbg_assert_msg_(DYNA_REC, false, "Invalid type in GetData");
			break;
		}
		return 0;
	}
};

class ARM64XEmitter
{
	friend class ARM64FloatEmitter;

private:
	u8* m_code;
	u8* m_startcode;
	u8* m_lastCacheFlushEnd;

	void EncodeCompareBranchInst(u32 op, ARM64Reg Rt, const void* ptr);
	void EncodeTestBranchInst(u32 op, ARM64Reg Rt, u8 bits, const void* ptr);
	void EncodeUnconditionalBranchInst(u32 op, const void* ptr);
	void EncodeUnconditionalBranchInst(u32 opc, u32 op2, u32 op3, u32 op4, ARM64Reg Rn);
	void EncodeExceptionInst(u32 instenc, u32 imm);
	void EncodeSystemInst(u32 op0, u32 op1, u32 CRn, u32 CRm, u32 op2, ARM64Reg Rt);
	void EncodeArithmeticInst(u32 instenc, bool flags, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void EncodeArithmeticCarryInst(u32 op, bool flags, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EncodeCondCompareImmInst(u32 op, ARM64Reg Rn, u32 imm, u32 nzcv, CCFlags cond);
	void EncodeCondCompareRegInst(u32 op, ARM64Reg Rn, ARM64Reg Rm, u32 nzcv, CCFlags cond);
	void EncodeCondSelectInst(u32 instenc, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);
	void EncodeData1SrcInst(u32 instenc, ARM64Reg Rd, ARM64Reg Rn);
	void EncodeData2SrcInst(u32 instenc, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EncodeData3SrcInst(u32 instenc, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void EncodeLogicalInst(u32 instenc, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void EncodeLoadRegisterInst(u32 bitop, ARM64Reg Rt, u32 imm);
	void EncodeLoadStoreExcInst(u32 instenc, ARM64Reg Rs, ARM64Reg Rt2, ARM64Reg Rn, ARM64Reg Rt);
	void EncodeLoadStorePairedInst(u32 op, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, u32 imm);
	void EncodeLoadStoreIndexedInst(u32 op, u32 op2, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void EncodeLoadStoreIndexedInst(u32 op, ARM64Reg Rt, ARM64Reg Rn, s32 imm, u8 size);
	void EncodeMOVWideInst(u32 op, ARM64Reg Rd, u32 imm, ShiftAmount pos);
	void EncodeBitfieldMOVInst(u32 op, ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void EncodeLoadStoreRegisterOffset(u32 size, u32 opc, ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void EncodeAddSubImmInst(u32 op, bool flags, u32 shift, u32 imm, ARM64Reg Rn, ARM64Reg Rd);
	void EncodeLogicalImmInst(u32 op, ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void EncodeLoadStorePair(u32 op, u32 load, IndexType type, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, s32 imm);
	void EncodeAddressInst(u32 op, ARM64Reg Rd, s32 imm);

protected:
	inline void Write32(u32 value)
	{
		*(u32*)m_code = value;
		m_code += 4;
	}

public:
	ARM64XEmitter()
		: m_code(nullptr), m_startcode(nullptr), m_lastCacheFlushEnd(nullptr)
	{
	}

	ARM64XEmitter(u8* code_ptr) {
		m_code = code_ptr;
		m_lastCacheFlushEnd = code_ptr;
		m_startcode = code_ptr;
	}

	virtual ~ARM64XEmitter()
	{
	}

	void SetCodePtr(u8* ptr);
	void ReserveCodeSpace(u32 bytes);
	const u8* AlignCode16();
	const u8* AlignCodePage();
	const u8* GetCodePtr() const;
	void FlushIcache();
	void FlushIcacheSection(u8* start, u8* end);
	u8* GetWritableCodePtr();

	// FixupBranch branching
	void SetJumpTarget(FixupBranch const& branch);
	FixupBranch CBZ(ARM64Reg Rt);
	FixupBranch CBNZ(ARM64Reg Rt);
	FixupBranch B(CCFlags cond);
	FixupBranch TBZ(ARM64Reg Rt, u8 bit);
	FixupBranch TBNZ(ARM64Reg Rt, u8 bit);
	FixupBranch B();
	FixupBranch BL();

	// Compare and Branch
	void CBZ(ARM64Reg Rt, const void* ptr);
	void CBNZ(ARM64Reg Rt, const void* ptr);

	// Conditional Branch
	void B(CCFlags cond, const void* ptr);

	// Test and Branch
	void TBZ(ARM64Reg Rt, u8 bits, const void* ptr);
	void TBNZ(ARM64Reg Rt, u8 bits, const void* ptr);

	// Unconditional Branch
	void B(const void* ptr);
	void BL(const void* ptr);

	// Unconditional Branch (register)
	void BR(ARM64Reg Rn);
	void BLR(ARM64Reg Rn);
	void RET(ARM64Reg Rn);
	void ERET();
	void DRPS();

	// Exception generation
	void SVC(u32 imm);
	void HVC(u32 imm);
	void SMC(u32 imm);
	void BRK(u32 imm);
	void HLT(u32 imm);
	void DCPS1(u32 imm);
	void DCPS2(u32 imm);
	void DCPS3(u32 imm);

	// System
	void _MSR(PStateField field, u8 imm);
	void HINT(SystemHint op);
	void CLREX();
	void DSB(BarrierType type);
	void DMB(BarrierType type);
	void ISB(BarrierType type);

	// Add/Subtract (Extended/Shifted register)
	void ADD(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ADD(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void ADDS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ADDS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void SUB(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void SUB(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void SUBS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void SUBS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void CMN(ARM64Reg Rn, ARM64Reg Rm);
	void CMN(ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);
	void CMP(ARM64Reg Rn, ARM64Reg Rm);
	void CMP(ARM64Reg Rn, ARM64Reg Rm, ArithOption Option);

	// Add/Subtract (with carry)
	void ADC(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ADCS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void SBC(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void SBCS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);

	// Conditional Compare (immediate)
	void CCMN(ARM64Reg Rn, u32 imm, u32 nzcv, CCFlags cond);
	void CCMP(ARM64Reg Rn, u32 imm, u32 nzcv, CCFlags cond);

	// Conditional Compare (register)
	void CCMN(ARM64Reg Rn, ARM64Reg Rm, u32 nzcv, CCFlags cond);
	void CCMP(ARM64Reg Rn, ARM64Reg Rm, u32 nzcv, CCFlags cond);

	// Conditional Select
	void CSEL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);
	void CSINC(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);
	void CSINV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);
	void CSNEG(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);

	// Data-Processing 1 source
	void RBIT(ARM64Reg Rd, ARM64Reg Rn);
	void REV16(ARM64Reg Rd, ARM64Reg Rn);
	void REV32(ARM64Reg Rd, ARM64Reg Rn);
	void REV64(ARM64Reg Rd, ARM64Reg Rn);
	void CLZ(ARM64Reg Rd, ARM64Reg Rn);
	void CLS(ARM64Reg Rd, ARM64Reg Rn);

	// Data-Processing 2 source
	void UDIV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void SDIV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void LSLV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void LSRV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ASRV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void RORV(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32B(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32H(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32W(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32CB(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32CH(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32CW(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32X(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void CRC32CX(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);

	// Data-Processing 3 source
	void MADD(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void MSUB(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void SMADDL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void SMSUBL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void SMULH(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void UMADDL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void UMSUBL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ARM64Reg Ra);
	void UMULH(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void MUL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void MNEG(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);

	// Logical (shifted register)
	void AND(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void BIC(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void ORR(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void ORN(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void EOR(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void EON(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void ANDS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void BICS(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, ArithOption Shift);
	void MOV(ARM64Reg Rd, ARM64Reg Rm);
	void MVN(ARM64Reg Rd, ARM64Reg Rm);

	// Logical (immediate)
	void AND(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void ANDS(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void EOR(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void ORR(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void TST(ARM64Reg Rn, u32 immr, u32 imms);

	// Add/subtract (immediate)
	void ADD(ARM64Reg Rd, ARM64Reg Rn, u32 imm, bool shift = false);
	void ADDS(ARM64Reg Rd, ARM64Reg Rn, u32 imm, bool shift = false);
	void SUB(ARM64Reg Rd, ARM64Reg Rn, u32 imm, bool shift = false);
	void SUBS(ARM64Reg Rd, ARM64Reg Rn, u32 imm, bool shift = false);
	void CMP(ARM64Reg Rn, u32 imm, bool shift = false);

	// Data Processing (Immediate)
	void MOVZ(ARM64Reg Rd, u32 imm, ShiftAmount pos = SHIFT_0);
	void MOVN(ARM64Reg Rd, u32 imm, ShiftAmount pos = SHIFT_0);
	void MOVK(ARM64Reg Rd, u32 imm, ShiftAmount pos = SHIFT_0);

	// Bitfield move
	void BFM(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void SBFM(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void UBFM(ARM64Reg Rd, ARM64Reg Rn, u32 immr, u32 imms);
	void SXTB(ARM64Reg Rd, ARM64Reg Rn);
	void SXTH(ARM64Reg Rd, ARM64Reg Rn);
	void SXTW(ARM64Reg Rd, ARM64Reg Rn);
	void UXTB(ARM64Reg Rd, ARM64Reg Rn);
	void UXTH(ARM64Reg Rd, ARM64Reg Rn);

	// Load Register (Literal)
	void LDR(ARM64Reg Rt, u32 imm);
	void LDRSW(ARM64Reg Rt, u32 imm);
	void PRFM(ARM64Reg Rt, u32 imm);

	// Load/Store Exclusive
	void STXRB(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void STLXRB(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void LDXRB(ARM64Reg Rt, ARM64Reg Rn);
	void LDAXRB(ARM64Reg Rt, ARM64Reg Rn);
	void STLRB(ARM64Reg Rt, ARM64Reg Rn);
	void LDARB(ARM64Reg Rt, ARM64Reg Rn);
	void STXRH(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void STLXRH(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void LDXRH(ARM64Reg Rt, ARM64Reg Rn);
	void LDAXRH(ARM64Reg Rt, ARM64Reg Rn);
	void STLRH(ARM64Reg Rt, ARM64Reg Rn);
	void LDARH(ARM64Reg Rt, ARM64Reg Rn);
	void STXR(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void STLXR(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rn);
	void STXP(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn);
	void STLXP(ARM64Reg Rs, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn);
	void LDXR(ARM64Reg Rt, ARM64Reg Rn);
	void LDAXR(ARM64Reg Rt, ARM64Reg Rn);
	void LDXP(ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn);
	void LDAXP(ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn);
	void STLR(ARM64Reg Rt, ARM64Reg Rn);
	void LDAR(ARM64Reg Rt, ARM64Reg Rn);

	// Load/Store no-allocate pair (offset)
	void STNP(ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, u32 imm);
	void LDNP(ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, u32 imm);

	// Load/Store register (immediate indexed)
	void STRB(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDRB(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDRSB(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void STRH(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDRH(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDRSH(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void STR(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDR(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void LDRSW(IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);

	// Load/Store register (register offset)
	void STRB(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDRB(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDRSB(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void STRH(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDRH(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDRSH(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void STR(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDR(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void LDRSW(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);
	void PRFM(ARM64Reg Rt, ARM64Reg Rn, ArithOption Rm);

	// Load/Store pair
	void LDP(IndexType type, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, s32 imm);
	void LDPSW(IndexType type, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, s32 imm);
	void STP(IndexType type, ARM64Reg Rt, ARM64Reg Rt2, ARM64Reg Rn, s32 imm);

	// Address of label/page PC-relative
	void ADR(ARM64Reg Rd, s32 imm);
	void ADRP(ARM64Reg Rd, s32 imm);

	// Wrapper around MOVZ+MOVK
	void MOVI2R(ARM64Reg Rd, u64 imm, bool optimize = true);

	// ABI related
	void ABI_PushRegisters(BitSet32 registers);
	void ABI_PopRegisters(BitSet32 registers, BitSet32 ignore_mask = BitSet32(0));

	// Utility to generate a call to a std::function object.
	//
	// Unfortunately, calling operator() directly is undefined behavior in C++
	// (this method might be a thunk in the case of multi-inheritance) so we
	// have to go through a trampoline function.
	template <typename T, typename... Args>
	static void CallLambdaTrampoline(const std::function<T(Args...)>* f,
	                                 Args... args)
	{
		(*f)(args...);
	}

	// This function expects you to have set up the state.
	// Overwrites X0 and X30
	template <typename T, typename... Args>
	ARM64Reg ABI_SetupLambda(const std::function<T(Args...)>* f)
	{
		auto trampoline = &ARM64XEmitter::CallLambdaTrampoline<T, Args...>;
		MOVI2R(X30, (u64)trampoline);
		MOVI2R(X0, (u64)const_cast<void*>((const void*)f));
		return X30;
	}
};

class ARM64FloatEmitter
{
public:
	ARM64FloatEmitter(ARM64XEmitter* emit) : m_emit(emit) {}

	void LDR(u8 size, IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void STR(u8 size, IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);

	// Loadstore single structure
	void LD1(u8 size, ARM64Reg Rt, u8 index, ARM64Reg Rn);
	void LD1(u8 size, ARM64Reg Rt, u8 index, ARM64Reg Rn, ARM64Reg Rm);
	void LD1R(u8 size, ARM64Reg Rt, ARM64Reg Rn);
	void ST1(u8 size, ARM64Reg Rt, u8 index, ARM64Reg Rn);
	void ST1(u8 size, ARM64Reg Rt, u8 index, ARM64Reg Rn, ARM64Reg Rm);

	// Loadstore multiple structure
	void LD1(u8 size, u8 count, ARM64Reg Rt, ARM64Reg Rn);

	// Scalar - 1 Source
	void FABS(ARM64Reg Rd, ARM64Reg Rn);
	void FNEG(ARM64Reg Rd, ARM64Reg Rn);

	// Scalar - 2 Source
	void FADD(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FMUL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FSUB(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);

	// Scalar floating point immediate
	void FMOV(ARM64Reg Rd, u32 imm);

	// Vector
	void AND(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void BSL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void DUP(u8 size, ARM64Reg Rd, ARM64Reg Rn, u8 index);
	void FABS(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FADD(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FCVTL(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCVTN(u8 dest_size, ARM64Reg Rd, ARM64Reg Rn);
	void FCVTZS(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCVTZU(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FDIV(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FMUL(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FNEG(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FRSQRTE(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FSUB(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void NOT(ARM64Reg Rd, ARM64Reg Rn);
	void ORR(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void REV16(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void REV32(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void REV64(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void SCVTF(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void UCVTF(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void XTN(u8 dest_size, ARM64Reg Rd, ARM64Reg Rn);

	// Move
	void DUP(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void INS(u8 size, ARM64Reg Rd, u8 index, ARM64Reg Rn);
	void INS(u8 size, ARM64Reg Rd, u8 index1, ARM64Reg Rn, u8 index2);
	void UMOV(u8 size, ARM64Reg Rd, ARM64Reg Rn, u8 index);
	void SMOV(u8 size, ARM64Reg Rd, ARM64Reg Rn, u8 index);

	// One source
	void FCVT(u8 size_to, u8 size_from, ARM64Reg Rd, ARM64Reg Rn);

	// Conversion between float and integer
	void FMOV(u8 size, bool top, ARM64Reg Rd, ARM64Reg Rn);
	void SCVTF(ARM64Reg Rd, ARM64Reg Rn);
	void UCVTF(ARM64Reg Rd, ARM64Reg Rn);

	// Float comparison
	void FCMP(ARM64Reg Rn, ARM64Reg Rm);
	void FCMP(ARM64Reg Rn);
	void FCMPE(ARM64Reg Rn, ARM64Reg Rm);
	void FCMPE(ARM64Reg Rn);
	void FCMEQ(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FCMEQ(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCMGE(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FCMGE(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCMGT(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void FCMGT(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCMLE(u8 size, ARM64Reg Rd, ARM64Reg Rn);
	void FCMLT(u8 size, ARM64Reg Rd, ARM64Reg Rn);

	// Conditional select
	void FCSEL(ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm, CCFlags cond);

	// Permute
	void UZP1(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void TRN1(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ZIP1(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void UZP2(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void TRN2(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void ZIP2(u8 size, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);

	// Shift by immediate
	void SSHLL(u8 src_size, ARM64Reg Rd, ARM64Reg Rn, u32 shift);
	void USHLL(u8 src_size, ARM64Reg Rd, ARM64Reg Rn, u32 shift);
	void SHRN(u8 dest_size, ARM64Reg Rd, ARM64Reg Rn, u32 shift);
	void SXTL(u8 src_size, ARM64Reg Rd, ARM64Reg Rn);
	void UXTL(u8 src_size, ARM64Reg Rd, ARM64Reg Rn);

	// ABI related
	void ABI_PushRegisters(BitSet32 registers);
	void ABI_PopRegisters(BitSet32 registers, BitSet32 ignore_mask = BitSet32(0));

private:
	ARM64XEmitter* m_emit;
	inline void Write32(u32 value) { m_emit->Write32(value); }

	// Emitting functions
	void EmitLoadStoreImmediate(u8 size, u32 opc, IndexType type, ARM64Reg Rt, ARM64Reg Rn, s32 imm);
	void Emit2Source(bool M, bool S, u32 type, u32 opcode, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EmitThreeSame(bool U, u32 size, u32 opcode, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EmitCopy(bool Q, u32 op, u32 imm5, u32 imm4, ARM64Reg Rd, ARM64Reg Rn);
	void Emit2RegMisc(bool U, u32 size, u32 opcode, ARM64Reg Rd, ARM64Reg Rn);
	void EmitLoadStoreSingleStructure(bool L, bool R, u32 opcode, bool S, u32 size, ARM64Reg Rt, ARM64Reg Rn);
	void EmitLoadStoreSingleStructure(bool L, bool R, u32 opcode, bool S, u32 size, ARM64Reg Rt, ARM64Reg Rn, ARM64Reg Rm);
	void Emit1Source(bool M, bool S, u32 type, u32 opcode, ARM64Reg Rd, ARM64Reg Rn);
	void EmitConversion(bool sf, bool S, u32 type, u32 rmode, u32 opcode, ARM64Reg Rd, ARM64Reg Rn);
	void EmitCompare(bool M, bool S, u32 op, u32 opcode2, ARM64Reg Rn, ARM64Reg Rm);
	void EmitCondSelect(bool M, bool S, CCFlags cond, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EmitPermute(u32 size, u32 op, ARM64Reg Rd, ARM64Reg Rn, ARM64Reg Rm);
	void EmitScalarImm(bool M, bool S, u32 type, u32 imm5, ARM64Reg Rd, u32 imm);
	void EmitShiftImm(bool U, u32 immh, u32 immb, u32 opcode, ARM64Reg Rd, ARM64Reg Rn);
	void EmitLoadStoreMultipleStructure(u32 size, bool L, u32 opcode, ARM64Reg Rt, ARM64Reg Rn);
	void EmitScalar1Source(bool M, bool S, u32 type, u32 opcode, ARM64Reg Rd, ARM64Reg Rn);
};

class ARM64CodeBlock : public CodeBlock<ARM64XEmitter>
{
private:
	void PoisonMemory() override
	{
		u32* ptr = (u32*)region;
		u32* maxptr = (u32*)region + region_size;
		// If our memory isn't a multiple of u32 then this won't write the last remaining bytes with anything
		// Less than optimal, but there would be nothing we could do but throw a runtime warning anyway.
		// AArch64: 0xD4200000 = BRK 0
		while (ptr < maxptr)
			*ptr++ = 0xD4200000;
	}
};
}

