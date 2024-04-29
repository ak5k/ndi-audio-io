#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button,
                              const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 6.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

        auto baseColour = // backgroundColour;
            backgroundColour
                .withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.0f
                                                                        : 0.9f)
                .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour =
                baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour(baseColour);

        auto flatOnLeft = button.isConnectedOnLeft();
        auto flatOnRight = button.isConnectedOnRight();
        auto flatOnTop = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
        {
            Path path;
            path.addRoundedRectangle(
                bounds.getX(), bounds.getY(), bounds.getWidth(),
                bounds.getHeight(), cornerSize, cornerSize,
                !(flatOnLeft || flatOnTop), !(flatOnRight || flatOnTop),
                !(flatOnLeft || flatOnBottom), !(flatOnRight || flatOnBottom));

            g.fillPath(path);

            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.strokePath(path, PathStrokeType(1.0f));
        }
        else
        {
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
        }
    }

    Font getComboBoxFont(ComboBox& /*box*/) override
    {
        return getCommonMenuFont();
    }

    Font getPopupMenuFont() override
    {
        return getCommonMenuFont();
    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        (void)buttonHeight;
        return getCommonMenuFont();
    }

  private:
    Font getCommonMenuFont()
    {
        return Font(Font::getDefaultMonospacedFontName(), "Regular", 13.f);
    }
};
