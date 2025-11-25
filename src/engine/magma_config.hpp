#pragma once

#if defined(MAGMA_WITH_EDITOR)
  constexpr bool kEditorEnabled = true;
#else
  constexpr bool kEditorEnabled = false;
#endif
