#pragma once
#include <functional>
namespace vox {
enum class PageId {
  Home,
  Voices,
  Effects,
  Soundboard,
  Favorites,
  Equalizer,
  Devices,
  Integrations,
  Presets,
  Diagnostics,
  Settings,
  Admin
};
class PageRouter {
public:
  using Listener = std::function<void(PageId, PageId)>;
  explicit PageRouter(PageId initial = PageId::Home)
      : current(initial), previous(initial) {}
  void navigateTo(PageId page);
  void back();
  PageId currentPage() const noexcept { return current; }
  PageId previousPage() const noexcept { return previous; }
  void setListener(Listener l) { listener = std::move(l); }
  static const char *title(PageId) noexcept;

private:
  PageId current, previous;
  Listener listener;
};
} // namespace vox
