if not exist bin (
	mkdir bin
)

gcc -o ./bin/renderer.exe ./src/main.c ./src/utils.c ./src/materials.c ./src/objects.c ./src/factory.c ./src/generation.c ./src/libs/perlin.c ^
-lgdi32 -lopengl32 -lglew32 -lglfw3 -mwindows ^
-I"..\..\librairies\stb_image" ^
-I"..\..\librairies\glew-2.1.0\include" ^
-I"..\..\librairies\glfw-3.3.8.bin.WIN64\glfw-3.3.8.bin.WIN64\include" ^
-I"..\..\librairies\cglm-master\cglm-master\include" ^
-L"..\..\librairies\glew-2.1.0\lib\Release\x64" ^
-L"..\..\librairies\glfw-3.3.8.bin.WIN64\glfw-3.3.8.bin.WIN64\lib-mingw-w64"

if exist bin/assets (
	DEL /Q "bin/assets"
)
xcopy /e /i /y assets "bin/assets"
