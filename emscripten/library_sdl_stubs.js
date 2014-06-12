//"use strict";

// Emscripten's SDL library doesn't (as of 1.16.0) cover everything that
// Gigalomania uses, so this library covers the gaps until Emscripten's SDL
// support improves (they're keen to use the actual C SDL library, ported to
// Emscripten, in the future).

var LibrarySDLStubs = {
  SDL_GetRGBA: function(pixel, format, ptr_r, ptr_g, ptr_b) {
    Runtime.warnOnce('SDL_GetRGBA has not been implemented (though I expect it would be easy)');
    // the actual implementation would look something along the lines of:
    // var r_value = (rgba&0xff);
    // var g_value = (rgba>>8 & 0xff);
    // var b_value = (rgba>>16 & 0xff);
    // var a_value = (rgba>>>24);
    //
    // {{{ makeSetValue('ptr_r', 0, 'r_value', 'i8') }}}
    // {{{ makeSetValue('ptr_g', 0, 'g_value', 'i8') }}}
    // {{{ makeSetValue('ptr_b', 0, 'b_value', 'i8') }}}
    return 0;
  },

  SDL_DisplayFormat__deps: ['SDL_DisplayFormatAlpha'],
  SDL_DisplayFormat: function(surf) {
    return _SDL_DisplayFormatAlpha(surf);
  },

  Mix_LoadMUSType_RW__deps: ['Mix_LoadMUS_RW'],
  Mix_LoadMUSType_RW: function(rwopsID, desiredType) {
    Runtime.warnOnce("Emscripten's SDL always autodetects music type, applications can use MixLoadMUS instead of MixLoadMUSType");
    return _Mix_LoadMUS_RW(rwopsID);
  },

  SDL_CreateCursor: function() {
    Runtime.warnOnce('SDL_CreateCursor has not been implemented');
    return 0;
  },

  SDL_FreeCursor: function() {
    Runtime.warnOnce('SDL_FreeCursor has not been implemented');
    return 0;
  },

  SDL_SetCursor: function() {
    Runtime.warnOnce('SDL_SetCursor has not been implemented');
    return 0;
  },

  SDL_GetCursor: function() {
    Runtime.warnOnce('SDL_GetCursor has not been implemented');
    return 0;
  }
}


//autoAddDeps(LibrarySDLStubs, '$SDLStubs');
mergeInto(LibraryManager.library, LibrarySDLStubs);

