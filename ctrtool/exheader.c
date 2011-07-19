#include <stdio.h>
#include <string.h>

#include "types.h"
#include "exheader.h"
#include "utils.h"
#include "ncch.h"

void exheader_init(exheader_context* ctx)
{
	memset(ctx, 0, sizeof(exheader_context));
}

void exheader_set_file(exheader_context* ctx, FILE* file)
{
	ctx->file = file;
}

void exheader_set_offset(exheader_context* ctx, u32 offset)
{
	ctx->offset = offset;
}

void exheader_set_size(exheader_context* ctx, u32 size)
{
	ctx->size = size;
}

void exheader_set_key(exheader_context* ctx, u8 key[16])
{
	memcpy(ctx->key, key, 16);
}

void exheader_set_partitionid(exheader_context* ctx, u8 partitionid[8])
{
	memcpy(ctx->partitionid, partitionid, 8);
}

void exheader_set_programid(exheader_context* ctx, u8 programid[8])
{
	memcpy(ctx->programid, programid, 8);
}

int exheader_get_compressedflag(exheader_context* ctx)
{
	return ctx->compressedflag;
}


int exheader_process(exheader_context* ctx, u32 actions)
{
	fseek(ctx->file, ctx->offset, SEEK_SET);
	fread(&ctx->header, 1, sizeof(exheader_header), ctx->file);


	ctr_init_ncch(&ctx->aes, ctx->key, ctx->partitionid, NCCHTYPE_EXHEADER);
	ctr_crypt_counter(&ctx->aes, (u8*)&ctx->header, (u8*)&ctx->header, sizeof(exheader_header));

	if (ctx->header.codesetinfo.flags.flag & 1)
		ctx->compressedflag = 1;

	if (memcmp(ctx->header.arm11systemlocalcaps.programid, ctx->programid, 8))
	{
		fprintf(stderr, "Error, program id mismatch. Wrong key?\n");
		return 0;
	}

	if (actions & InfoFlag)
	{
		exheader_print(ctx);		
	}

	return 1;
}

void exheader_print(exheader_context* ctx)
{
	u32 i;
	char name[9];
	char service[9];
	exheader_codesetinfo* codesetinfo = &ctx->header.codesetinfo;

	memset(name, 0, sizeof(name));
	memcpy(name, codesetinfo->name, 8);

	fprintf(stdout, "\nExtended header:\n");
	fprintf(stdout, "Name:                   %s\n", name);
	fprintf(stdout, "Flag:                   %02X ", codesetinfo->flags.flag);
	if (codesetinfo->flags.flag & 1)
		fprintf(stdout, "[compressed]");
	fprintf(stdout, "\n");
	fprintf(stdout, "Remaster version:       %04X\n", getle16(codesetinfo->flags.remasterversion));

	fprintf(stdout, "Code text address:      0x%08X\n", getle32(codesetinfo->text.address));
	fprintf(stdout, "Code text size:         0x%08X\n", getle32(codesetinfo->text.codesize));
	fprintf(stdout, "Code text max pages:    0x%08X (0x%08X)\n", getle32(codesetinfo->text.nummaxpages), getle32(codesetinfo->text.nummaxpages)*0x1000);
	fprintf(stdout, "Code ro address:        0x%08X\n", getle32(codesetinfo->ro.address));
	fprintf(stdout, "Code ro size:           0x%08X\n", getle32(codesetinfo->ro.codesize));
	fprintf(stdout, "Code ro max pages:      0x%08X (0x%08X)\n", getle32(codesetinfo->ro.nummaxpages), getle32(codesetinfo->ro.nummaxpages)*0x1000);
	fprintf(stdout, "Code data address:      0x%08X\n", getle32(codesetinfo->data.address));
	fprintf(stdout, "Code data size:         0x%08X\n", getle32(codesetinfo->data.codesize));
	fprintf(stdout, "Code data max pages:    0x%08X (0x%08X)\n", getle32(codesetinfo->data.nummaxpages), getle32(codesetinfo->data.nummaxpages)*0x1000);
	fprintf(stdout, "Code bss size:          0x%08X\n", getle32(codesetinfo->bsssize));
	fprintf(stdout, "Code stack size:        0x%08X\n", getle32(codesetinfo->stacksize));

	for(i=0; i<0x30; i++)
	{
		if (getle64(ctx->header.deplist.programid[i]) != 0x0000000000000000UL)
			fprintf(stdout, "Dependency:             %016llX\n", getle64(ctx->header.deplist.programid[i]));
	}

	fprintf(stdout, "Savedata size:          0x%08X\n", getle32(ctx->header.systeminfo.savedatasize));
	fprintf(stdout, "Jump id:                %016llX\n", getle64(ctx->header.systeminfo.jumpid));

	fprintf(stdout, "Program id:             %016llX\n", getle64(ctx->header.arm11systemlocalcaps.programid));
	memdump(stdout, "Flags:                  ", ctx->header.arm11systemlocalcaps.flags, 8);
	// print resource limit descriptor too? currently mostly zeroes...
	fprintf(stdout, "Ext savedata id:        %016llX\n", getle64(ctx->header.arm11systemlocalcaps.storageinfo.extsavedataid));
	fprintf(stdout, "System savedata id:     %016llX\n", getle64(ctx->header.arm11systemlocalcaps.storageinfo.systemsavedataid));
	memdump(stdout, "Access info:            ", ctx->header.arm11systemlocalcaps.storageinfo.accessinfo, 7);
	fprintf(stdout, "Other attributes:       %02X\n", ctx->header.arm11systemlocalcaps.storageinfo.otherattributes);
	
	for(i=0; i<0x20; i++)
	{
		if (getle64(ctx->header.arm11systemlocalcaps.serviceaccesscontrol[i]) != 0x0000000000000000UL)
		{
			memset(service, 0, sizeof(service));
			memcpy(service, ctx->header.arm11systemlocalcaps.serviceaccesscontrol[i], 8);
			fprintf(stdout, "Service access:         %s\n", service);
		}
	}
	fprintf(stdout, "Reslimit category       %02X\n", ctx->header.arm11systemlocalcaps.resourcelimitcategory);
}