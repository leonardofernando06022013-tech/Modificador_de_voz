#include "PageRouter.h"
namespace vox {
void PageRouter::navigateTo(PageId page) {
  if (page == current)
    return;
  auto old = current;
  previous = current;
  current = page;
  if (listener)
    listener(old, page);
}
void PageRouter::back() {
  if (previous == current)
    return;
  auto target = previous;
  previous = current;
  auto old = current;
  current = target;
  if (listener)
    listener(old, current);
}
const char *PageRouter::title(PageId p) noexcept {
  switch (p) {
  case PageId::Home:
    return "Inicio";
  case PageId::Voices:
    return "Biblioteca de Vozes";
  case PageId::Effects:
    return "Biblioteca de Efeitos";
  case PageId::Soundboard:
    return "Painel de Som";
  case PageId::Favorites:
    return "Favoritos";
  case PageId::Equalizer:
    return "Equalizador";
  case PageId::Devices:
    return "Dispositivos";
  case PageId::Integrations:
    return "Integracoes";
  case PageId::Presets:
    return "Presets";
  case PageId::Diagnostics:
    return "Diagnostico";
  case PageId::Settings:
    return "Configuracoes";
  case PageId::Admin:
    return "Painel de Administracao";
  }
  return "";
}
} // namespace vox
