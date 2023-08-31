#include <stdio.h>
#include <string.h>

int
main(int argc, char** argv)
{
	if (argc != 3) fprintf(stderr, "ERROR: Invalid number of arguments. Expected: pack_shaders <name_of_resulting_string> <shader_file_path>\n");
	else
	{
		FILE* shader_file;
		if (fopen_s(&shader_file, argv[2], "rb") != 0) fprintf(stderr, "ERROR: Failed to open shader file\n");
		else
		{
			printf("static GLchar* %s = \n", argv[1]);

			char buffer[1024];
			while (fgets(buffer, sizeof(buffer), shader_file))
			{
				printf("  \"");
				for (char* scan = buffer; *scan != 0 && *scan != '\r' && *scan != '\n'; ++scan)
				{
					if (*scan == '"') printf("\\\"");
					else              putchar(*scan);
				}
				printf("\\n\"\n");
			}

			printf(";\n\n");
		}

		fclose(shader_file);
	}
	return 0;
}
