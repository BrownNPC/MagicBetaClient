package sdl

const (
	GL_CONTEXT_PROFILE_CORE          = 0x0001 /**< OpenGL Core Profile context */
	GL_CONTEXT_PROFILE_COMPATIBILITY = 0x0002 /**< OpenGL Compatibility Profile context */
	GL_CONTEXT_PROFILE_ES            = 0x0004 /**< GLX_CONTEXT_ES2_PROFILE_BIT_EXT */
)
const (
	GL_RED_SIZE                   = iota /**< the minimum number of bits for the red channel of the color buffer; defaults to 8. */
	GL_GREEN_SIZE                        /**< the minimum number of bits for the green channel of the color buffer; defaults to 8. */
	GL_BLUE_SIZE                         /**< the minimum number of bits for the blue channel of the color buffer; defaults to 8. */
	GL_ALPHA_SIZE                        /**< the minimum number of bits for the alpha channel of the color buffer; defaults to 8. */
	GL_BUFFER_SIZE                       /**< the minimum number of bits for frame buffer size; defaults to 0. */
	GL_DOUBLEBUFFER                      /**< whether the output is single or double buffered; defaults to double buffering on. */
	GL_DEPTH_SIZE                        /**< the minimum number of bits in the depth buffer; defaults to 16. */
	GL_STENCIL_SIZE                      /**< the minimum number of bits in the stencil buffer; defaults to 0. */
	GL_ACCUM_RED_SIZE                    /**< the minimum number of bits for the red channel of the accumulation buffer; defaults to 0. */
	GL_ACCUM_GREEN_SIZE                  /**< the minimum number of bits for the green channel of the accumulation buffer; defaults to 0. */
	GL_ACCUM_BLUE_SIZE                   /**< the minimum number of bits for the blue channel of the accumulation buffer; defaults to 0. */
	GL_ACCUM_ALPHA_SIZE                  /**< the minimum number of bits for the alpha channel of the accumulation buffer; defaults to 0. */
	GL_STEREO                            /**< whether the output is stereo 3D; defaults to off. */
	GL_MULTISAMPLEBUFFERS                /**< the number of buffers used for multisample anti-aliasing; defaults to 0. */
	GL_MULTISAMPLESAMPLES                /**< the number of samples used around the current pixel used for multisample anti-aliasing. */
	GL_ACCELERATED_VISUAL                /**< set to 1 to require hardware acceleration, set to 0 to force software rendering; defaults to allow either. */
	GL_RETAINED_BACKING                  /**< not used (deprecated). */
	GL_CONTEXT_MAJOR_VERSION             /**< OpenGL context major version. */
	GL_CONTEXT_MINOR_VERSION             /**< OpenGL context minor version. */
	GL_CONTEXT_FLAGS                     /**< some combination of 0 or more of elements of the SDL_GLContextFlag enumeration; defaults to 0. */
	GL_CONTEXT_PROFILE_MASK              /**< type of GL context (Core, Compatibility, ES). See SDL_GLProfile; default value depends on platform. */
	GL_SHARE_WITH_CURRENT_CONTEXT        /**< OpenGL context sharing; defaults to 0. */
	GL_FRAMEBUFFER_SRGB_CAPABLE          /**< requests sRGB capable visual; defaults to 0. */
	GL_CONTEXT_RELEASE_BEHAVIOR          /**< sets context the release behavior. See SDL_GLContextReleaseFlag; defaults to FLUSH. */
	GL_CONTEXT_RESET_NOTIFICATION        /**< set context reset notification. See SDL_GLContextResetNotification; defaults to NO_NOTIFICATION. */
	GL_CONTEXT_NO_ERROR
	GL_FLOATBUFFERS
	GL_EGL_PLATFORM
)
