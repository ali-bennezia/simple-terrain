if not exist bin (
	mkdir bin
)

gcc -o ./bin/renderer.exe ./src/main.c ./src/utils.c ./src/materials.c ./src/objects.c ./src/factory.c ./src/noises.c ./src/generator.c ./libs/perlin/perlin.c ^
-lgdi32 -lopengl32 -lglew32 -lglfw3 -mwindows -lpthread ^
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
