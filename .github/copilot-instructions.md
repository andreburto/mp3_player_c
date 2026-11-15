# GitHub Copilot Instructions

# .c Files

For C files, follow the project's coding style guidelines, including indentation, naming conventions, and comment style.

Use descriptive variable and function names, and ensure that all functions have clear and concise comments explaining their purpose and usage.

Prefer using `const` and `static` where appropriate to indicate immutability and limited scope.
Avoid magic numbers; use named constants instead.
Ensure proper error handling and resource management, especially for memory allocation and file operations.

Follow the project's guidelines for code formatting, including indentation, spacing, and brace placement, to maintain consistency and readability.

Use meaningful comments to explain complex logic, but avoid redundant comments that state the obvious.

Ensure that header files have proper include guards to prevent multiple inclusions.
Organize code into logical modules and separate interface from implementation to improve maintainability.

Use windows.h for Windows-specific functionality and ensure proper inclusion order to avoid conflicts with other headers.

All files go in the appropriate source or header directories, typically `src` for implementation files and `include` for header files, to maintain a clear project structure.