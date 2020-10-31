CC=../tcc/tcc

game.exe: game.c palette.h
	$(CC) game.c -o game.exe -lgdi32
	
.PHONY: clean

clean:
	del game.exe
