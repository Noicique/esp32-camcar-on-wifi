1.Variable Naming Convention
Local Variables: snake_case (e.g., frame_buffer, retry_count).
Global Variables: g_ prefix followed by snake_case (e.g., g_system_state, g_wifi_connected).
Static Variables (File Scope): s_ prefix (e.g., s_camera_initialized).
Constants/Macros: ALL_CAPS_SNAKE_CASE (e.g., MAX_RETRY_COUNT, PIN_LED).
Structs/Classes: PascalCase (e.g., CameraManager, ImageData).
Class Members: m_ prefix (e.g., m_width, m_buffer) or simple snake_case if private.
Pointers: Explicitly denote pointers (e.g., uint8_t* p_frame_data).
2.Code Integrity & Modularity
Separation of Concerns: Hardware logic (drivers) must be separated from Business logic (application).
Header Files: Only expose public interfaces in .h files. Keep internal states and helper functions static in .cpp files.
Code Completeness: Never use // ... rest of code. Generate the full function body. If the code is too long, split it into logical blocks but ensure each block is complete.
