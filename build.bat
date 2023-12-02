set "debug=true"

set "depflag=-lpthread -lopengl32 -lglu32 -lgdi32 -luser32 -lkernel32"
set "debugflag="

if %debug% == false (
	set "debugflag=-mwindows"
)

if not exist bin (
	mkdir bin
)

gcc -o ./bin/renderer.exe ./src/main.c ./src/utils.c ./src/materials.c ./src/objects.c ./src/factory.c ./src/noises.c ./src/generator.c ./src/renderer.c ./src/terrain.c ^
./src/vbopools.c ./src/mempools.c ./src/debug.c ^
./libs/perlin/perlin.c ^
-lglew32 -lglfw3 %debugflag%  %depflag% ^
-I".\libs\stb_image" ^
-I".\libs\glew-2.1.0\include" ^
-I".\libs\glfw-3.3.8.bin.WIN64\glfw-3.3.8.bin.WIN64\include" ^
-I".\libs\cglm-master\cglm-master\include" ^
-L".\libs\glew-2.1.0\lib\Release\x64" ^
-L".\libs\glfw-3.3.8.bin.WIN64\glfw-3.3.8.bin.WIN64\lib-mingw-w64"

if exist bin/assets (
	DEL /Q "bin/assets"
)
xcopy /e /i /y assets "bin/assets"
