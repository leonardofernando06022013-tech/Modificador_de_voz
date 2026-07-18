#include "AppLookAndFeel.h"
#include "Theme.h"
namespace vox {
AppLookAndFeel::AppLookAndFeel(){setColour(juce::TextEditor::backgroundColourId,theme::elevated);setColour(juce::TextEditor::textColourId,theme::text);setColour(juce::TextEditor::outlineColourId,theme::border);setColour(juce::TextEditor::focusedOutlineColourId,theme::blue);setColour(juce::ComboBox::backgroundColourId,theme::elevated);setColour(juce::ComboBox::textColourId,theme::text);setColour(juce::PopupMenu::backgroundColourId,theme::elevated);setColour(juce::PopupMenu::textColourId,theme::text);setColour(juce::TooltipWindow::backgroundColourId,theme::elevated);setColour(juce::TooltipWindow::textColourId,theme::text);setColour(juce::TooltipWindow::outlineColourId,theme::border);}
void AppLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                          const juce::Colour &colour,
                                          bool hover, bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(1.0f);
  auto fill = colour == juce::Colours::transparentBlack ? theme::panel : colour;
  if (b.getComponentID() == "danger") fill = theme::red.withAlpha(.82f);
  if (b.getComponentID() == "success") fill = theme::green.withAlpha(.82f);
  if (!b.isEnabled()) fill = theme::panel.withAlpha(.58f);
  if (hover && b.isEnabled() && !theme::reduceAnimations.load())
    fill = fill.brighter(.10f);
  if (down && b.isEnabled()) r = r.reduced(1.5f);

  if (b.getComponentID() == "powerButton") {
    juce::ColourGradient powerFill(fill.brighter(.12f), r.getX(), r.getY(),
                                   fill.darker(.18f), r.getRight(),
                                   r.getBottom(), false);
    g.setGradientFill(powerFill);
    g.fillEllipse(r);
    g.setColour(theme::cyan.withAlpha(b.isEnabled() ? .65f : .18f));
    g.drawEllipse(r.reduced(.75f), 1.5f);
    return;
  }

  juce::DropShadow(juce::Colours::black.withAlpha(.22f), 5, {0, 2})
      .drawForRectangle(g, r.getSmallestIntegerContainer());
  g.setColour(fill);
  g.fillRoundedRectangle(r, theme::smallRadius);
  const bool focused = theme::focusVisible.load() && b.hasKeyboardFocus(true);
  g.setColour(focused ? theme::cyan
                      : theme::highContrast.load() ? theme::muted
                                                   : theme::border);
  g.drawRoundedRectangle(r, theme::smallRadius, focused ? 2.0f : 1.0f);
}
void AppLookAndFeel::drawButtonText(juce::Graphics&g,juce::TextButton&b,bool,bool){g.setColour(b.isEnabled()?theme::text:theme::disabled);g.setFont(juce::Font(juce::FontOptions(14.0f,juce::Font::bold)));g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(8,2),juce::Justification::centred,1);}
void AppLookAndFeel::drawLinearSlider(juce::Graphics&g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle,juce::Slider&){auto cy=y+h*.5f;g.setColour(theme::border);g.fillRoundedRectangle((float)x,cy-3,(float)w,6,3);g.setColour(theme::blue);g.fillRoundedRectangle((float)x,cy-3,juce::jmax(0.0f,pos-x),6,3);g.setColour(theme::cyan);g.fillEllipse(pos-7,cy-7,14,14);}
void AppLookAndFeel::drawToggleButton(juce::Graphics&g,juce::ToggleButton&b,bool hover,bool){auto r=b.getLocalBounds().toFloat();auto sw=juce::Rectangle<float>(r.getRight()-45,r.getCentreY()-11,40,22);g.setColour(b.getToggleState()?theme::blue:(hover?theme::border.brighter(.1f):theme::border));g.fillRoundedRectangle(sw,11);g.setColour(theme::text);g.fillEllipse(b.getToggleState()?sw.getRight()-19:sw.getX()+3,sw.getY()+3,16,16);g.setFont(14);g.drawText(b.getButtonText(),r.withTrimmedRight(52),juce::Justification::centredLeft);}
void AppLookAndFeel::drawComboBox(juce::Graphics&g,int w,int h,bool,int,int,int,int,juce::ComboBox&){g.setColour(theme::elevated);g.fillRoundedRectangle(0,0,(float)w,(float)h,8);g.setColour(theme::border);g.drawRoundedRectangle(0,0,(float)w,(float)h,8,1);juce::Path p;p.addTriangle((float)w-20,(float)h*.42f,(float)w-10,(float)h*.42f,(float)w-15,(float)h*.62f);g.setColour(theme::muted);g.fillPath(p);}
}
