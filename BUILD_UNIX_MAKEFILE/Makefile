AR = ar
CC = gcc
CFLAGS_TOOLS = -O3 -Wall
CFLAGS_LIB = -O3 -Wall
CFLAGS_OTHER = -O3 -Wall
LDFLAGS_TOOLS = -ldjvupure
RM = rm -f

all: djvupuretree djvupuremake djvupureinsert

djvupuretree: libdjvupure.a djvupuretree.o wmain_stdc.o
	$(CC) $(CFLAGS) djvupuretree.o wmain_stdc.o $(LDFLAGS_TOOLS) -o djvupuretree

djvupureinsert: libdjvupure.a djvupureinsert.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupureinsert.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupureinsert

djvupuremake: libdjvupure.a djvupureinsert.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o
	$(CC) $(CFLAGS) djvupuremake.o aux_create.o aux_insert.o wmain_stdc.o wtoi.o $(LDFLAGS_TOOLS) -o djvupuremake

libdjvupure.a: djvupure_container.o djvupure_core.o djvupure_document.o djvupure_info.o djvupure_io.o djvupure_page.o djvupure_raw.o
	$(AR) rcs libdjvupure.a $^

%.o: ../src/tools/%.c
	$(CC) -c $(CFLAGS_TOOLS) $< -o $@

%.o: ../src/%.c
	$(CC) -c $(CFLAGS_LIB) $< -o $@

%.o : ../src/unixsupport/%.c
	$(CC) -c $(CFLAGS_OTHER) $< -o $@

clean:
	$(RM) djvupuretree djvupureinsert djvupuremake libdjvupure.a *.o