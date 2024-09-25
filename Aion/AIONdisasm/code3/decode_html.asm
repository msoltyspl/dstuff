
signed int __cdecl sub_10079510(char *FullPath, uint8_t *buf, int length)
{
  int v3; // ebx@1
  signed int result; // eax@2
  int i; // eax@4
  char v6; // cl@4
  int sum; // edi@4
  int xorval; // esi@4
  int v9; // eax@6
  int v10; // esi@6
  int v11; // edi@8
  int v12; // ecx@9
  int cnt; // edi@9
  int v14; // ecx@5
  char Drive; // [sp+230h] [bp-34h]@4
  char Dir; // [sp+4h] [bp-260h]@4
  char Filename[252]; // [sp+100h] [bp-164h]@4
  char Ext; // [sp+1FCh] [bp-68h]@4


  if ( len < 2 || buf[0] != 0x81 )
	return 0; //apparently fine

  v3 = buf;

    sum = 0;
    xorval = 0;
    splitpath(FullPath, &Drive, &Dir, Filename, &Ext);

	for(i=0;i<strlen(filename);i++) {

		seed = i + (filename[i] & 0xf);
		tmp += seed;
		xorval ^= seed;
	}


    tmp = xorval ^ (tmp + 0x1d);
    xorval += 3;

    if( (uint8_t)tmp ^ buf[1] != 0x81)
	return -1;

	if(len > 2) {
	        cnt = len - 2;
		ptr = buf;
		do {
			tmp = xorval ^ (tmp + 0x1d);
			xorval += 3;
			*ptr = (uint8_t)tmp ^ ptr[2];
			ptr += 2;
		} while(--cnt);
	}

      *(_BYTE *)(buf + len - 1) = 0;
      *(_BYTE *)(buf + len - 2) = 0;
      result = 1;


  return result;
}


0x78710
@Game_2D129510:                              ;<= Procedure Start

        PUSH EBP
        MOV EBP, ESP
        SUB ESP, 0260h
        CMP DWORD PTR SS:[EBP+010h], 2
        PUSH EBX
        MOV EBX, DWORD PTR SS:[EBP+0Ch]
        JGE @Game_2D12952D

@Game_2D129523:

        MOV EAX, 1
        POP EBX
        MOV ESP, EBP
        POP EBP
        RETN

@Game_2D12952D:

        CMP BYTE PTR DS:[EBX], 081h
        JNZ @Game_2D129523
        PUSH ESI
        PUSH EDI
        LEA EAX, DWORD PTR SS:[EBP-068h]
        PUSH EAX
        LEA ECX, DWORD PTR SS:[EBP-0164h]
        PUSH ECX
        MOV ECX, DWORD PTR SS:[EBP+8]
        LEA EDX, DWORD PTR SS:[EBP-0260h]
        PUSH EDX
        LEA EAX, DWORD PTR SS:[EBP-034h]
        PUSH EAX
        PUSH ECX
        XOR EDI, EDI
        XOR ESI, ESI
        CALL DWORD PTR DS:[<&msvcr80._splitpath>] ; MSVCR80._splitpath
        MOVZX ECX, BYTE PTR SS:[EBP-0164h]
        ADD ESP, 014h
        XOR EAX, EAX
        TEST CL, CL
        JE @Game_2D12957F

@Game_2D129568:

        AND ECX, 0Fh
        ADD ECX, EAX
        ADD EAX, 1
        ADD EDI, ECX
        XOR ESI, ECX
        MOV CL, BYTE PTR SS:[EBP+EAX-0164h]
        TEST CL, CL
        JNZ @Game_2D129568

@Game_2D12957F:

        MOVZX EDX, BYTE PTR DS:[EBX+1]
        LEA EAX, DWORD PTR DS:[EDI+01Dh]
        XOR EAX, ESI
        XOR EDX, EAX
        ADD ESI, 3
        CMP DL, 081h
        JE @Game_2D12959B
        POP EDI
        POP ESI
        XOR EAX, EAX
        POP EBX
        MOV ESP, EBP
        POP EBP
        RETN

@Game_2D12959B:

        MOV EDI, DWORD PTR SS:[EBP+010h]
        CMP EDI, 2
        JLE @Game_2D1295C2
        MOV ECX, EBX
        ADD EDI, -2

@Game_2D1295A8:

        MOV DL, BYTE PTR DS:[ECX+2]
        ADD EAX, 01Dh
        XOR EAX, ESI
        XOR DL, AL
        MOV BYTE PTR DS:[ECX], DL
        ADD ESI, 3
        ADD ECX, 1
        SUB EDI, 1
        JNZ @Game_2D1295A8
        MOV EDI, DWORD PTR SS:[EBP+010h]

@Game_2D1295C2:

        MOV BYTE PTR DS:[EBX+EDI-1], 0
        MOV BYTE PTR DS:[EBX+EDI-2], 0
        POP EDI
        POP ESI
        MOV EAX, 1
        POP EBX
        MOV ESP, EBP
        POP EBP
        RETN                                 ;<= Procedure End

