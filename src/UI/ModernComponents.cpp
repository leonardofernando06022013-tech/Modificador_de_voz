#include "ModernComponents.h"
namespace vox {
void AudioLevelMeter::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat();
  auto labels = r.removeFromBottom(13);
  const float level = juce::jmax(in, out);
  const float db =
      juce::jmax(-60.0f, 20.0f * std::log10(juce::jmax(level, .001f)));
  const float fraction = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
  constexpr int segments = 22;
  const float gap = 2.0f,
              width = (r.getWidth() - gap * (segments - 1)) / segments;
  for (int i = 0; i < segments; ++i) {
    auto segment = juce::Rectangle<float>(
        r.getX() + i * (width + gap), r.getY() + 3, width, r.getHeight() - 6);
    const float position = (float)(i + 1) / segments;
    auto colour = position > .9f
                      ? theme::red
                      : (position > .72f ? theme::yellow : theme::green);
    g.setColour(position <= fraction ? colour : theme::border.withAlpha(.55f));
    g.fillRoundedRectangle(segment, 1.5f);
  }
  g.setColour(peak >= 1.0f ? theme::red : theme::muted);
  g.setFont(8.5f);
  g.drawText("-60     -40      -20      -10   -6  -3   0",
             labels.toNearestInt(), juce::Justification::centredLeft);
  if (peak >= 1.0f)
    g.drawText("CLIP", getLocalBounds().removeFromRight(30),
               juce::Justification::centredRight);
}
VoiceCardComponent::VoiceCardComponent(juce::String n, juce::String c, int i)
    : juce::Button(n), name(std::move(n)), category(std::move(c)),
      colourIndex(i) {
  setTooltip("Aplicar " + name +
             juce::String::fromUTF8(". Use o coração para favoritar."));
  setWantsKeyboardFocus(true);
}
void VoiceCardComponent::drawAvatar(juce::Graphics &g,
                                    juce::Rectangle<float> a) {
  const juce::Colour colours[]{theme::cyan,
                               theme::purple,
                               juce::Colour(0xffff4fa3),
                               theme::green,
                               theme::yellow,
                               theme::blue,
                               theme::red,
                               theme::green,
                               theme::purple,
                               juce::Colour(0xffaab6ca),
                               juce::Colour(0xffffa84d),
                               juce::Colour(0xffbd67ff)};
  auto accent = colours[colourIndex % 12];
  g.setColour(accent.withAlpha(.16f));
  g.fillEllipse(a);
  g.setColour(accent);
  g.drawEllipse(a, 2);
  auto c = a.reduced(12);
  g.setColour(theme::background.withAlpha(.88f));
  g.fillEllipse(c);
  switch (colourIndex % 18) {
  case 0: {
    g.setColour(accent);
    g.drawRoundedRectangle(c.getCentreX() - 6, c.getY() + 5, 12, 25, 6, 3);
    juce::Path arc;
    arc.addArc(c.getX() + 8, c.getY() + 15, c.getWidth() - 16, 22, 0,
               juce::MathConstants<float>::pi, true);
    g.strokePath(arc, juce::PathStrokeType(2));
    g.drawLine(c.getCentreX(), c.getBottom() - 12, c.getCentreX(),
               c.getBottom() - 5, 2);
    break;
  }
  case 1:
  case 2:
  case 11:
  case 15: {
    g.setColour(accent);
    g.fillEllipse(c.getCentreX() - 12, c.getY() + 6, 24, 24);
    juce::Path shoulders;
    shoulders.addRoundedRectangle(c.getX() + 5, c.getBottom() - 18,
                                  c.getWidth() - 10, 20, 9);
    g.fillPath(shoulders);
    g.setColour(theme::text);
    g.fillEllipse(c.getCentreX() - 7, c.getY() + 16, 3, 3);
    g.fillEllipse(c.getCentreX() + 4, c.getY() + 16, 3, 3);
    break;
  }
  case 3:
  case 6:
  case 17: {
    g.setColour(accent);
    g.fillRoundedRectangle(c.reduced(4, 8), 6);
    g.setColour(theme::background);
    g.fillRoundedRectangle(c.getX() + 9, c.getY() + 15, c.getWidth() - 18, 12,
                           5);
    g.setColour(theme::cyan);
    g.fillEllipse(c.getCentreX() - 9, c.getCentreY() - 3, 5, 5);
    g.fillEllipse(c.getCentreX() + 4, c.getCentreY() - 3, 5, 5);
    break;
  }
  case 4: {
    g.setColour(accent);
    g.drawRoundedRectangle(c.reduced(3, 10), 4, 3);
    for (int y = 14; y < 30; y += 6)
      g.drawHorizontalLine((int)c.getY() + y, c.getX() + 8, c.getRight() - 8);
    break;
  }
  case 5: {
    g.setColour(accent);
    g.drawEllipse(c.reduced(6, 4), 3);
    juce::Path arc;
    arc.addArc(c.getX() + 10, c.getY() + 11, c.getWidth() - 20,
               c.getHeight() - 22, -.8f, .8f, true);
    g.strokePath(arc, juce::PathStrokeType(3));
    break;
  }
  case 7: {
    g.setColour(accent);
    juce::Path head;
    head.addEllipse(c.reduced(5, 7));
    g.fillPath(head);
    g.setColour(theme::background);
    g.fillEllipse(c.getCentreX() - 10, c.getCentreY() - 4, 7, 10);
    g.fillEllipse(c.getCentreX() + 3, c.getCentreY() - 4, 7, 10);
    break;
  }
  case 8: {
    g.setColour(accent);
    juce::Path horn;
    horn.addTriangle(c.getX() + 5, c.getCentreY(), c.getRight() - 5,
                     c.getY() + 7, c.getRight() - 5, c.getBottom() - 7);
    g.fillPath(horn);
    g.drawLine(c.getX() + 5, c.getCentreY(), c.getX(), c.getCentreY(), 4);
    break;
  }
  case 9: {
    g.setColour(accent);
    g.fillRoundedRectangle(c.reduced(5, 4), 8);
    g.setColour(theme::background);
    g.fillRoundedRectangle(c.getX() + 10, c.getY() + 13, c.getWidth() - 20, 8,
                           4);
    break;
  }
  case 10: {
    g.setColour(accent);
    g.fillEllipse(c.reduced(7));
    g.setColour(theme::text);
    g.fillEllipse(c.getCentreX() - 9, c.getCentreY() - 5, 5, 6);
    g.fillEllipse(c.getCentreX() + 4, c.getCentreY() - 5, 5, 6);
    juce::Path smile;
    smile.addArc(c.getCentreX() - 10, c.getCentreY(), 20, 12, 0,
                 juce::MathConstants<float>::pi, true);
    g.strokePath(smile, juce::PathStrokeType(2));
    break;
  }
  default: {
    juce::Path wave;
    for (int i = 0; i < 5; ++i) {
      float x = c.getX() + 6.0f + i * 7.0f, h = 9.0f + (i % 3) * 8.0f;
      wave.startNewSubPath(x, c.getCentreY() - h * .5f);
      wave.lineTo(x, c.getCentreY() + h * .5f);
    }
    g.setColour(accent);
    g.strokePath(wave, juce::PathStrokeType(3));
    break;
  }
  }
}
void VoiceCardComponent::paintButton(juce::Graphics &g, bool hover, bool down) {
  auto r = getLocalBounds().toFloat().reduced(3);
  g.setColour(selected ? theme::elevated.brighter(.05f)
                       : (hover ? theme::elevated : theme::panel));
  g.fillRoundedRectangle(r, 11);
  if (hover) {
    g.setColour(theme::purple.withAlpha(.13f));
    g.fillRoundedRectangle(r.reduced(2), 10);
  }
  g.setColour(selected
                  ? theme::purple.brighter(.2f)
                  : (hover ? theme::border.brighter(.25f) : theme::border));
  g.drawRoundedRectangle(r, 11, selected ? 2.0f : 1.0f);
  auto avatar =
      juce::Rectangle<float>(r.getCentreX() - 38, r.getY() + 16, 76, 76);
  if (hover)
    avatar = avatar.expanded(1.2f);
  drawAvatar(g, avatar);
  if (selected) {
    g.setColour(theme::blue);
    g.fillEllipse(r.getX() + 9, r.getY() + 9, 18, 18);
    g.setColour(theme::text);
    g.setFont(11);
    g.drawText("OK", (int)r.getX() + 9, (int)r.getY() + 9, 18, 18,
               juce::Justification::centred);
  }
  g.setColour(favourite ? juce::Colour(0xffff4fa3) : theme::muted);
  auto heart = juce::Rectangle<float>(r.getRight() - 26, r.getY() + 12, 15, 14);
  juce::Path heartPath;
  heartPath.startNewSubPath(heart.getCentreX(), heart.getBottom());
  heartPath.cubicTo(heart.getX() - 2, heart.getCentreY(), heart.getX() + 2,
                    heart.getY(), heart.getCentreX(), heart.getY() + 4);
  heartPath.cubicTo(heart.getRight() - 2, heart.getY(), heart.getRight() + 2,
                    heart.getCentreY(), heart.getCentreX(), heart.getBottom());
  if (favourite)
    g.fillPath(heartPath);
  else
    g.strokePath(heartPath, juce::PathStrokeType(1.5f));
  g.setColour(theme::text);
  g.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
  g.drawFittedText(name,
                   r.withTrimmedTop(102).withTrimmedBottom(27).toNearestInt(),
                   juce::Justification::centred, 2);
  g.setColour(theme::muted);
  g.setFont(11);
  g.drawText(
      category,
      r.withTrimmedTop(r.getHeight() - 28).withTrimmedRight(22).toNearestInt(),
      juce::Justification::centred);
  g.setFont(16);
  g.drawText(juce::String::fromUTF8("⋮"), (int)r.getRight() - 23,
             (int)r.getBottom() - 27, 18, 20, juce::Justification::centred);
  if (down) {
    g.setColour(theme::background.withAlpha(.15f));
    g.fillRoundedRectangle(r, 11);
  }
}
void VoiceCardComponent::mouseUp(const juce::MouseEvent &e) {
  if (e.position.x > getWidth() - 38 && e.position.y < 38) {
    favourite = !favourite;
    if (onFavourite)
      onFavourite(favourite);
    repaint();
    return;
  }
  juce::Button::mouseUp(e);
}
SidebarComponent::SidebarComponent() {
  const juce::StringArray labels{juce::String::fromUTF8("⌂  Início"),
                                 juce::String::fromUTF8("♩  Vozes"),
                                 juce::String::fromUTF8("✦  Efeitos"),
                                 juce::String::fromUTF8("▦  Painel de Som"),
                                 juce::String::fromUTF8("♡  Favoritos"),
                                 juce::String::fromUTF8("≋  Equalizador"),
                                 juce::String::fromUTF8("◉  Dispositivos"),
                                 juce::String::fromUTF8("↗  Integrações"),
                                 juce::String::fromUTF8("□  Presets"),
                                 juce::String::fromUTF8("⌁  Diagnóstico"),
                                 juce::String::fromUTF8("⚙  Configurações"),
                                 juce::String::fromUTF8("▣  Admin"),
                                 "?  Ajuda",
                                 juce::String::fromUTF8("ⓘ  Sobre")};
  for (int i = 0; i < labels.size(); ++i) {
    auto *b = buttons.add(new juce::TextButton(labels[i]));
    b->setTooltip(labels[i]);
    b->setColour(juce::TextButton::buttonColourId,
                 juce::Colours::transparentBlack);
    b->setColour(juce::TextButton::textColourOffId, theme::muted);
    b->setColour(juce::TextButton::textColourOnId, theme::text);
    b->onClick = [this, i] {
      if (onSelect)
        onSelect(i);
    };
    addAndMakeVisible(b);
  }
  updateTexts();
}
void SidebarComponent::updateTexts() {
  auto &l = LocalizationManager::instance();
  const juce::StringArray icons{
      juce::String::fromUTF8("⌂  "), juce::String::fromUTF8("♪  "),
      juce::String::fromUTF8("✦  "), juce::String::fromUTF8("▦  "),
      juce::String::fromUTF8("♡  "), juce::String::fromUTF8("≋  "),
      juce::String::fromUTF8("◉  "), juce::String::fromUTF8("↗  "),
      juce::String::fromUTF8("□  "), juce::String::fromUTF8("⌁  "),
      juce::String::fromUTF8("⚙  "), juce::String::fromUTF8("▣  "),
      "?  ", juce::String::fromUTF8("ⓘ  ")};
  const juce::StringArray keys{"nav.home","nav.voices","nav.effects","nav.soundboard","nav.favorites","nav.equalizer","nav.devices","nav.integrations","nav.presets","nav.diagnostics","nav.settings","nav.admin","nav.help","nav.about"};
  for (int i = 0; i < buttons.size() && i < keys.size(); ++i) {
    const auto label = l.text(keys[i]);
    const auto value = compact ? icons[i].trimEnd() : icons[i] + label;
    buttons[i]->setButtonText(value);
    buttons[i]->setTooltip(label);
  }
}
void SidebarComponent::setSelected(int i) {
  selected = i;
  for (int n = 0; n < buttons.size(); ++n)
    buttons[n]->setColour(juce::TextButton::buttonColourId,
                          n == i ? theme::blue
                                 : juce::Colours::transparentBlack);
  repaint();
}
void SidebarComponent::resized() {
  auto a = getLocalBounds().reduced(compact ? 7 : 10, 14);
  for (int i = 0; i < buttons.size(); ++i) {
    if (i == 11)
      a = getLocalBounds().removeFromBottom(138).reduced(compact ? 7 : 10, 6);
    buttons[i]->setBounds(a.removeFromTop(38).reduced(2));
    a.removeFromTop(3);
  }
}
void SidebarComponent::paint(juce::Graphics &g) {
  g.fillAll(theme::secondary);
  g.setColour(theme::border);
  g.drawLine((float)getWidth() - 1, 0, (float)getWidth() - 1,
             (float)getHeight());
  if (selected >= 0 && selected < buttons.size()) {
    auto bounds = buttons[selected]->getBounds().toFloat();
    g.setColour(theme::purple.withAlpha(.22f));
    g.fillRoundedRectangle(bounds, 8);
    g.setColour(theme::cyan);
    g.fillRoundedRectangle(0.0f, bounds.getY() + 3, 4.0f,
                           bounds.getHeight() - 6, 2.0f);
    g.setColour(theme::border.brighter(.15f));
    g.drawRoundedRectangle(bounds, 8, 1);
  }
}
NotificationComponent::NotificationComponent() {
  setInterceptsMouseClicks(false, false);
  setVisible(false);
}
void NotificationComponent::showMessage(const juce::String &m, bool e) {
  message = m;
  colour = e ? theme::red : theme::green;
  setVisible(true);
  toFront(false);
  startTimer(e ? 6000 : 2400);
  repaint();
}
void NotificationComponent::timerCallback() {
  stopTimer();
  setVisible(false);
}
void NotificationComponent::paint(juce::Graphics &g) {
  g.setColour(theme::elevated);
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 10);
  g.setColour(colour);
  g.fillRoundedRectangle(0, 0, 5.0f, (float)getHeight(), 2);
  g.setColour(theme::text);
  g.setFont(14);
  g.drawFittedText(message, getLocalBounds().reduced(16, 8),
                   juce::Justification::centredLeft, 2);
}
} // namespace vox
