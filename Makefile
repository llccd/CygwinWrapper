binaries = scp.exe git.exe

all: $(binaries)

%.exe: %.c
	$(CC) -O3 -o $@ $^

.PHONY: clean
clean:
	$(RM) $(binaries)