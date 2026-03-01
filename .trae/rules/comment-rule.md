2.3 Comments
Use Doxygen style for function declarations in header files.
Inline comments should explain “Why”, not just “What”.
Example:
/**

@brief Initializes the camera hardware.
@return true if successful, false otherwise.
@note Requires PSRAM to be enabled for frame buffer allocation.
*/
bool init_camera();