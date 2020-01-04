# cpp-config-file-values-loader
Small C++ utility library to load values from a file at runtime.

This library is intended for people (me) who just want to store a few values in a file and don't want to use JSON or XML or any kind of file format that would be overkill for the job (and would also require a complex parser).

The file format is intended to be as simple as possible. Each value has to be on a separated line. If you wanted to store the window width of your app, you would have a line with this : 'window_width = 1200'.

The values identifiers (and their types) expected to be found in the file have to be specified at compile time, allowing the program to check if all values have been loaded correctly.

