#pragma once
#include <cstdint>
#include <cstring>

#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xfd
#define DELTA_FPU_MODRM    0x104
#define DELTA_PREFIXES     0x13c
#define DELTA_OP_LOCK_OK   0x1ae
#define DELTA_OP2_LOCK_OK  0x1c6
#define DELTA_OP_ONLY_MEM  0x1d8
#define DELTA_OP2_ONLY_MEM 0x1e7

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_IMM64         0x00000020
#define F_DISP8         0x00000040
#define F_DISP16        0x00000080
#define F_DISP32        0x00000100
#define F_RELATIVE      0x00000200
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_REX    0x40000000
#define F_PREFIX_ANY    0x7f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

namespace hde64
{
#pragma pack(push,1)
	struct result
	{
		uint8_t len;
		uint8_t p_rep;
		uint8_t p_lock;
		uint8_t p_seg;
		uint8_t p_66;
		uint8_t p_67;
		uint8_t rex;
		uint8_t rex_w;
		uint8_t rex_r;
		uint8_t rex_x;
		uint8_t rex_b;
		uint8_t opcode;
		uint8_t opcode2;
		uint8_t modrm;
		uint8_t modrm_mod;
		uint8_t modrm_reg;
		uint8_t modrm_rm;
		uint8_t sib;
		uint8_t sib_scale;
		uint8_t sib_index;
		uint8_t sib_base;
		
		union
		{
			uint8_t imm8;
			uint16_t imm16;
			uint32_t imm32;
			uint64_t imm64;
		} imm;
		uint8_t imm_offset;

		union
		{
			uint8_t disp8;
			uint16_t disp16;
			uint32_t disp32;
		} disp;
		uint8_t disp_offset;

		uint32_t flags;
	};
#pragma pack(pop)

	inline constexpr unsigned char table[] = {
		0xa5,0xaa,0xa5,0xb8,0xa5,0xaa,0xa5,0xaa,0xa5,0xb8,0xa5,0xb8,0xa5,0xb8,0xa5,
		0xb8,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xac,0xc0,0xcc,0xc0,0xa1,0xa1,
		0xa1,0xa1,0xb1,0xa5,0xa5,0xa6,0xc0,0xc0,0xd7,0xda,0xe0,0xc0,0xe4,0xc0,0xea,
		0xea,0xe0,0xe0,0x98,0xc8,0xee,0xf1,0xa5,0xd3,0xa5,0xa5,0xa1,0xea,0x9e,0xc0,
		0xc0,0xc2,0xc0,0xe6,0x03,0x7f,0x11,0x7f,0x01,0x7f,0x01,0x3f,0x01,0x01,0xab,
		0x8b,0x90,0x64,0x5b,0x5b,0x5b,0x5b,0x5b,0x92,0x5b,0x5b,0x76,0x90,0x92,0x92,
		0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x6a,0x73,0x90,
		0x5b,0x52,0x52,0x52,0x52,0x5b,0x5b,0x5b,0x5b,0x77,0x7c,0x77,0x85,0x5b,0x5b,
		0x70,0x5b,0x7a,0xaf,0x76,0x76,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,
		0x5b,0x5b,0x86,0x01,0x03,0x01,0x04,0x03,0xd5,0x03,0xd5,0x03,0xcc,0x01,0xbc,
		0x03,0xf0,0x03,0x03,0x04,0x00,0x50,0x50,0x50,0x50,0xff,0x20,0x20,0x20,0x20,
		0x01,0x01,0x01,0x01,0xc4,0x02,0x10,0xff,0xff,0xff,0x01,0x00,0x03,0x11,0xff,
		0x03,0xc4,0xc6,0xc8,0x02,0x10,0x00,0xff,0xcc,0x01,0x01,0x01,0x00,0x00,0x00,
		0x00,0x01,0x01,0x03,0x01,0xff,0xff,0xc0,0xc2,0x10,0x11,0x02,0x03,0x01,0x01,
		0x01,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0x10,
		0x10,0x10,0x10,0x02,0x10,0x00,0x00,0xc6,0xc8,0x02,0x02,0x02,0x02,0x06,0x00,
		0x04,0x00,0x02,0xff,0x00,0xc0,0xc2,0x01,0x01,0x03,0x03,0x03,0xca,0x40,0x00,
		0x0a,0x00,0x04,0x00,0x00,0x00,0x00,0x7f,0x00,0x33,0x01,0x00,0x00,0x00,0x00,
		0x00,0x00,0xff,0xbf,0xff,0xff,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0xff,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
		0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,
		0xff,0x40,0x40,0x40,0x40,0x41,0x49,0x40,0x40,0x40,0x40,0x4c,0x42,0x40,0x40,
		0x40,0x40,0x40,0x40,0x40,0x40,0x4f,0x44,0x53,0x40,0x40,0x40,0x44,0x57,0x43,
		0x5c,0x40,0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
		0x40,0x40,0x64,0x66,0x6e,0x6b,0x40,0x40,0x6a,0x46,0x40,0x40,0x44,0x46,0x40,
		0x40,0x5b,0x44,0x40,0x40,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x01,0x06,
		0x06,0x02,0x06,0x06,0x00,0x06,0x00,0x0a,0x0a,0x00,0x00,0x00,0x02,0x07,0x07,
		0x06,0x02,0x0d,0x06,0x06,0x06,0x0e,0x05,0x05,0x02,0x02,0x00,0x00,0x04,0x04,
		0x04,0x04,0x05,0x06,0x06,0x06,0x00,0x00,0x00,0x0e,0x00,0x00,0x08,0x00,0x10,
		0x00,0x18,0x00,0x20,0x00,0x28,0x00,0x30,0x00,0x80,0x01,0x82,0x01,0x86,0x00,
		0xf6,0xcf,0xfe,0x3f,0xab,0x00,0xb0,0x00,0xb1,0x00,0xb3,0x00,0xba,0xf8,0xbb,
		0x00,0xc0,0x00,0xc1,0x00,0xc7,0xbf,0x62,0xff,0x00,0x8d,0xff,0x00,0xc4,0xff,
		0x00,0xc5,0xff,0x00,0xff,0xff,0xeb,0x01,0xff,0x0e,0x12,0x08,0x00,0x13,0x09,
		0x00,0x16,0x08,0x00,0x17,0x09,0x00,0x2b,0x09,0x00,0xae,0xff,0x07,0xb2,0xff,
		0x00,0xb4,0xff,0x00,0xb5,0xff,0x00,0xc3,0x01,0x00,0xc7,0xff,0xbf,0xe7,0x08,
		0x00,0xf0,0x02,0x00
	};

	inline result disasm( const void* instruction )
	{
		result res;
		memset( &res, 0, sizeof( result ) );

		uint8_t x, c, * p = ( uint8_t* ) instruction, cflags, opcode, pref = 0;
		const uint8_t* ht = table;
		uint8_t m_mod, m_reg, m_rm, disp_size = 0;
		uint8_t op64 = 0;

		for( x = 16; x; x-- )
			switch( c = *p++ )
			{
				case 0xf3:
					res.p_rep = c;
					pref |= PRE_F3;
					break;
				case 0xf2:
					res.p_rep = c;
					pref |= PRE_F2;
					break;
				case 0xf0:
					res.p_lock = c;
					pref |= PRE_LOCK;
					break;
				case 0x26: case 0x2e: case 0x36:
				case 0x3e: case 0x64: case 0x65:
					res.p_seg = c;
					pref |= PRE_SEG;
					break;
				case 0x66:
					res.p_66 = c;
					pref |= PRE_66;
					break;
				case 0x67:
					res.p_67 = c;
					pref |= PRE_67;
					break;
				default:
					goto pref_done;
			}
	pref_done:

		res.flags = ( uint32_t ) pref << 23;

		if( !pref )
			pref |= PRE_NONE;

		if( ( c & 0xf0 ) == 0x40 )
		{
			res.flags |= F_PREFIX_REX;
			if( ( res.rex_w = ( c & 0xf ) >> 3 ) && ( *p & 0xf8 ) == 0xb8 )
				op64++;
			res.rex_r = ( c & 7 ) >> 2;
			res.rex_x = ( c & 3 ) >> 1;
			res.rex_b = c & 1;
			if( ( ( c = *p++ ) & 0xf0 ) == 0x40 )
			{
				opcode = c;
				goto error_opcode;
			}
		}

		if( ( res.opcode = c ) == 0x0f )
		{
			res.opcode2 = c = *p++;
			ht += DELTA_OPCODES;
		}
		else if( c >= 0xa0 && c <= 0xa3 )
		{
			op64++;
			if( pref & PRE_67 )
				pref |= PRE_66;
			else
				pref &= ~PRE_66;
		}

		opcode = c;
		cflags = ht[ ht[ opcode / 4 ] + ( opcode % 4 ) ];

		if( cflags == C_ERROR )
		{
		error_opcode:
			res.flags |= F_ERROR | F_ERROR_OPCODE;
			cflags = 0;
			if( ( opcode & -3 ) == 0x24 )
				cflags++;
		}

		x = 0;
		if( cflags & C_GROUP )
		{
			uint16_t t;
			t = *( uint16_t* ) ( ht + ( cflags & 0x7f ) );
			cflags = ( uint8_t ) t;
			x = ( uint8_t ) ( t >> 8 );
		}

		if( res.opcode2 )
		{
			ht = table + DELTA_PREFIXES;
			if( ht[ ht[ opcode / 4 ] + ( opcode % 4 ) ] & pref )
				res.flags |= F_ERROR | F_ERROR_OPCODE;
		}

		if( cflags & C_MODRM )
		{
			res.flags |= F_MODRM;
			res.modrm = c = *p++;
			res.modrm_mod = m_mod = c >> 6;
			res.modrm_rm = m_rm = c & 7;
			res.modrm_reg = m_reg = ( c & 0x3f ) >> 3;

			if( x && ( ( x << m_reg ) & 0x80 ) )
				res.flags |= F_ERROR | F_ERROR_OPCODE;

			if( !res.opcode2 && opcode >= 0xd9 && opcode <= 0xdf )
			{
				uint8_t t = opcode - 0xd9;
				if( m_mod == 3 )
				{
					ht = table + DELTA_FPU_MODRM + t * 8;
					t = ht[ m_reg ] << m_rm;
				}
				else
				{
					ht = table + DELTA_FPU_REG;
					t = ht[ t ] << m_reg;
				}
				if( t & 0x80 )
					res.flags |= F_ERROR | F_ERROR_OPCODE;
			}

			if( pref & PRE_LOCK )
			{
				if( m_mod == 3 )
				{
					res.flags |= F_ERROR | F_ERROR_LOCK;
				}
				else
				{
					const uint8_t* table_end;
					uint8_t op = opcode;
					if( res.opcode2 )
					{
						ht = table + DELTA_OP2_LOCK_OK;
						table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
					}
					else
					{
						ht = table + DELTA_OP_LOCK_OK;
						table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
						op &= -2;
					}
					for( ; ht != table_end; ht++ )
						if( *ht++ == op )
						{
							if( !( ( *ht << m_reg ) & 0x80 ) )
								goto no_lock_error;
							else
								break;
						}
					res.flags |= F_ERROR | F_ERROR_LOCK;
				no_lock_error:
					;
				}
			}

			if( res.opcode2 )
			{
				switch( opcode )
				{
					case 0x20: case 0x22:
						m_mod = 3;
						if( m_reg > 4 || m_reg == 1 )
							goto error_operand;
						else
							goto no_error_operand;
					case 0x21: case 0x23:
						m_mod = 3;
						if( m_reg == 4 || m_reg == 5 )
							goto error_operand;
						else
							goto no_error_operand;
				}
			}
			else
			{
				switch( opcode )
				{
					case 0x8c:
						if( m_reg > 5 )
							goto error_operand;
						else
							goto no_error_operand;
					case 0x8e:
						if( m_reg == 1 || m_reg > 5 )
							goto error_operand;
						else
							goto no_error_operand;
				}
			}

			if( m_mod == 3 )
			{
				const uint8_t* table_end;
				if( res.opcode2 )
				{
					ht = table + DELTA_OP2_ONLY_MEM;
					table_end = ht + sizeof( table ) - DELTA_OP2_ONLY_MEM;
				}
				else
				{
					ht = table + DELTA_OP_ONLY_MEM;
					table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
				}
				for( ; ht != table_end; ht += 2 )
					if( *ht++ == opcode )
					{
						if( *ht++ & pref && !( ( *ht << m_reg ) & 0x80 ) )
							goto error_operand;
						else
							break;
					}
				goto no_error_operand;
			}
			else if( res.opcode2 )
			{
				switch( opcode )
				{
					case 0x50: case 0xd7: case 0xf7:
						if( pref & ( PRE_NONE | PRE_66 ) )
							goto error_operand;
						break;
					case 0xd6:
						if( pref & ( PRE_F2 | PRE_F3 ) )
							goto error_operand;
						break;
					case 0xc5:
						goto error_operand;
				}
				goto no_error_operand;
			}
			else
				goto no_error_operand;

		error_operand:
			res.flags |= F_ERROR | F_ERROR_OPERAND;
		no_error_operand:

			c = *p++;
			if( m_reg <= 1 )
			{
				if( opcode == 0xf6 )
					cflags |= C_IMM8;
				else if( opcode == 0xf7 )
					cflags |= C_IMM_P66;
			}

			switch( m_mod )
			{
				case 0:
					if( pref & PRE_67 )
					{
						if( m_rm == 6 )
							disp_size = 2;
					}
					else
						if( m_rm == 5 )
							disp_size = 4;
					break;
				case 1:
					disp_size = 1;
					break;
				case 2:
					disp_size = 2;
					if( !( pref & PRE_67 ) )
						disp_size <<= 1;
			}

			if( m_mod != 3 && m_rm == 4 )
			{
				res.flags |= F_SIB;
				p++;
				res.sib = c;
				res.sib_scale = c >> 6;
				res.sib_index = ( c & 0x3f ) >> 3;
				if( ( res.sib_base = c & 7 ) == 5 && !( m_mod & 1 ) )
					disp_size = 4;
			}

			p--;
			switch( disp_size )
			{
				case 1:
					res.flags |= F_DISP8;
					res.disp.disp8 = *p;
					break;
				case 2:
					res.flags |= F_DISP16;
					res.disp.disp16 = *( uint16_t* ) p;
					break;
				case 4:
					res.flags |= F_DISP32;
					res.disp.disp32 = *( uint32_t* ) p;
			}
			res.disp_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
			p += disp_size;
		}
		else if( pref & PRE_LOCK )
			res.flags |= F_ERROR | F_ERROR_LOCK;

		if( cflags & C_IMM_P66 )
		{
			if( cflags & C_REL32 )
			{
				if( pref & PRE_66 )
				{
					res.flags |= F_IMM16 | F_RELATIVE;
					res.imm.imm16 = *( uint16_t* ) p;
					res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
					p += 2;
					goto disasm_done;
				}
				goto rel32_ok;
			}
			if( op64 )
			{
				res.flags |= F_IMM64;
				res.imm.imm64 = *( uint64_t* ) p;
				res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
				p += 8;
			}
			else if( !( pref & PRE_66 ) )
			{
				res.flags |= F_IMM32;
				res.imm.imm32 = *( uint32_t* ) p;
				res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
				p += 4;
			}
			else
				goto imm16_ok;
		}


		if( cflags & C_IMM16 )
		{
		imm16_ok:
			res.flags |= F_IMM16;
			res.imm.imm16 = *( uint16_t* ) p;
			res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
			p += 2;
		}
		if( cflags & C_IMM8 )
		{
			res.flags |= F_IMM8;
			res.imm.imm8 = *p;
			res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
			p += 1;
		}

		if( cflags & C_REL32 )
		{
		rel32_ok:
			res.flags |= F_IMM32 | F_RELATIVE;
			res.imm.imm32 = *( uint32_t* ) p;
			res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
			p += 4;
		}
		else if( cflags & C_REL8 )
		{
			res.flags |= F_IMM8 | F_RELATIVE;
			res.imm.imm8 = *p;
			res.imm_offset = ( uint8_t* ) p - ( uint8_t* ) instruction;
			p += 1;
		}

	disasm_done:

		if( ( res.len = ( uint8_t ) ( p - ( uint8_t* ) instruction ) ) > 15 )
		{
			res.flags |= F_ERROR | F_ERROR_LENGTH;
			res.len = 15;
		}

		return res;
	}
	inline result disasm( const void* begin, const void* end )
	{
		// 24 instead of 15 since prefix handling is a bit wrong.
		//
		size_t max_len = ( const char* ) end - ( const char* ) begin;
		if ( max_len > 24 )
			return disasm( begin );

		uint8_t bytes[ 24 ] = {};
		memcpy( bytes, begin, max_len > 24 ? 24 : max_len );
		result r = disasm( bytes );
		if ( r.len > max_len )
			r.flags |= F_ERROR | F_ERROR_LENGTH;
		return r;
	}
};