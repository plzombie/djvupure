AR = ar
CC = gcc
CFLAGS_TOOLS = -O3 -Wall
CFLAGS_LIB = -O3 -Wall
CFLAGS_OTHER = -O3 -Wall
LDFLAGS_TOOLS = -L. -ldjvupure -lm
RM = rm -f

all: djvupuretree djvupureinsert djvupuremake djvupurefix djvupureextract djvupuredec

djvupuretree: libdjvupure.a djvupuretree.o wmain_stdc.o
	$(CC) $(CFLAGS) djvupuretree.o wmain_stdc.o $(LDFLAGS_TOOLS) -o djvupuretree

djvupureinsert: libdjvupure.a djvupureinsert.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupureinsert.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupureinsert

djvupuremake: libdjvupure.a djvupuremake.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupuremake.o djvupure_sign.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupuremake

djvupurefix: libdjvupure.a djvupurefix.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupurefix.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupurefix

djvupureextract: libdjvupure.a djvupureextract.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupureextract.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupureextract

djvupuredec: libdjvupure.a djvupuredec.o ppm_save.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupuredec.o ppm_save.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupuredec
	
libdjvupure.a: djvupure_bgjp.o djvupure_container.o djvupure_core.o djvupure_dir.o djvupure_document.o djvupure_fgjp.o djvupure_image.o djvupure_info.o djvupure_io.o djvupure_jpeg.o djvupure_page.o djvupure_raw.o djvupure_sign.o djvupure_smmr.o wfopen.o wcstombsl.o
	$(AR) rcs libdjvupure.a $^

%.o: ../src/tools/%.c
	$(CC) -c $(CFLAGS_TOOLS) $< -o $@

%.o: ../src/all2ppm/src/%.c
	$(CC) -c $(CFLAGS_TOOLS) $< -o $@

%.o: ../src/%.c
	$(CC) -c $(CFLAGS_LIB) $< -o $@

%.o : ../src/unixsupport/%.c
	$(CC) -c $(CFLAGS_OTHER) $< -o $@

%.o : ../src/extclib/%.c
	$(CC) -c $(CFLAGS_OTHER) $< -o $@

clean:
	$(RM) djvupuretree djvupureinsert djvupuremake djvupurefix djvupureextract djvupuredec libdjvupure.a *.o
