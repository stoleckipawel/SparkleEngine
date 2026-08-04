#pragma once

#include <QtCore/QMargins>
#include <QtCore/QSize>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>

namespace SparkleLauncher::LauncherUi
{
	namespace Color
	{
		inline constexpr const char* Background = "#111312";
		inline constexpr const char* HeroBackground = "#030404";
		inline constexpr const char* Shell = "#181a19";
		inline constexpr const char* Panel = "#202220";
		inline constexpr const char* PanelHover = "#2c302c";
		inline constexpr const char* Field = "#202321";
		inline constexpr const char* Border = "#0b0d0c";
		inline constexpr const char* BorderSoft = "#303430";
		inline constexpr const char* BorderStrong = "#444943";
		inline constexpr const char* Divider = "#2b2f2b";
		inline constexpr const char* Accent = "#76b900";
		inline constexpr const char* AccentHover = "#8bd80f";
		inline constexpr const char* Selection = "#31451f";
		inline constexpr const char* TextPrimary = "#f2f4f1";
		inline constexpr const char* TextBody = "#d9ddd7";
		inline constexpr const char* TextSecondary = "#b9c0b6";
		inline constexpr const char* TextMuted = "#858d82";
		inline constexpr const char* StateQueued = "#8b949e";
		inline constexpr const char* StateRunning = Accent;
		inline constexpr const char* StateSuccess = "#7ee787";
		inline constexpr const char* StateDestructive = "#ff7b72";
		inline constexpr const char* StateWarning = "#ffb454";

		QColor Hex(const char* value, int alpha = 255);
	}

	namespace Space
	{
		inline constexpr int Tiny = 2;
		inline constexpr int XSmall = 4;
		inline constexpr int Small = 8;
		inline constexpr int Medium = 12;
		inline constexpr int Large = 16;
		inline constexpr int XLarge = 18;
		inline constexpr int SectionGap = 18;
	}

	namespace Window
	{
		inline constexpr int MinimumWidth = 1280;
		inline constexpr int MinimumHeight = 720;
		inline constexpr int InitialWidth = 1480;
		inline constexpr int InitialHeight = 860;
	}

	namespace Icon
	{
		inline constexpr int DefaultSize = 14;
	}

	namespace Shell
	{
		inline constexpr int RailWidth = 86;
		inline constexpr int RailItemMinHeight = 58;
		inline constexpr int TabMinHeight = 36;
		inline constexpr int RailIconSize = 18;
		inline constexpr int RailBottomPadding = 10;
		inline constexpr int RailGroupSpacing = 2;
		inline constexpr int WorkflowTabSpacing = 18;
		inline constexpr int PanelHorizontalMargin = 18;
		inline constexpr int PanelVerticalMargin = 14;
	}

	namespace TitleBand
	{
		inline constexpr int Spacing = 14;
		inline const QMargins Margins{20, 0, 12, 0};
	}

	namespace HeaderContext
	{
		inline constexpr int Spacing = 10;
		inline constexpr int ComboHeight = 28;
		inline constexpr int LevelComboMinWidth = 170;
		inline constexpr int LevelComboMaxWidth = 240;
		inline constexpr int GraphicsApiComboMinWidth = 100;
		inline constexpr int GraphicsApiComboMaxWidth = 130;
		inline constexpr int ConfigurationComboMinWidth = 140;
		inline constexpr int ConfigurationComboMaxWidth = 180;
		inline constexpr int IdeComboMinWidth = 120;
		inline constexpr int IdeComboMaxWidth = 150;
		inline constexpr int CompilerComboMinWidth = 100;
		inline constexpr int CompilerComboMaxWidth = 130;
	}

	namespace FooterContext
	{
		inline constexpr int Spacing = 10;
		inline const QMargins Margins{16, 5, 16, 5};
	}

	namespace Row
	{
		inline constexpr int FieldLabelWidth = 132;
		inline constexpr int FieldValueWidth = 560;
		inline constexpr int InlineStatusWidth = 88;
		inline constexpr int StatusActionWidth = 84;
		inline constexpr int StatusActionHeight = 26;
	}

	namespace Button
	{
		inline constexpr int PrimaryMinHeight = 34;
		inline constexpr int SecondaryMinHeight = 30;
	}

	namespace Page
	{
		inline constexpr int MaxContentWidth = 1560;
		inline constexpr int Spacing = 10;
		inline constexpr int BottomMargin = 32;
		inline const QMargins ContentMargins{28, 22, 28, BottomMargin};
		inline const QMargins QuickStartMargins{0, 0, 0, BottomMargin};
	}

	namespace ActionMeta
	{
		inline constexpr int Spacing = 8;
		inline constexpr int PrimaryButtonWidth = 144;
		inline constexpr int SecondaryButtonWidth = 76;
		inline constexpr int ButtonHeight = 32;
		inline const QMargins Margins{0, 8, Page::ContentMargins.right(), 8};
	}

	namespace TextEdit
	{
		inline constexpr int MinHeight = 78;
		inline constexpr int MaxHeight = 118;
	}

	namespace OperationOutput
	{
		inline constexpr int MinHeight = 96;
		inline constexpr int CompactMaxHeight = 128;
		inline constexpr int ProminentMinHeight = 136;
		inline constexpr int MaxHeight = 220;
	}

	namespace Clean
	{
		inline constexpr int RowSpacing = 12;
		inline const QMargins RowMargins{14, 10, 14, 10};
	}

	namespace Option
	{
		inline constexpr int LabelHorizontalPadding = 10;
		inline constexpr int ValueLeftPadding = 10;
		inline constexpr int GroupSpacing = 4;
		inline constexpr int StatusDetailSpacing = 1;

		inline const QMargins LabelMargins{LabelHorizontalPadding, 0, LabelHorizontalPadding, 0};
		inline const QMargins ValueMargins{ValueLeftPadding, 0, 0, 0};
		inline const QMargins GroupMargins{0, 6, 0, 6};
	}

	namespace WorkflowVisual
	{
		inline constexpr int MinHeight = 118;
		inline constexpr int FallbackWidth = 360;
		inline constexpr int FallbackHeight = 180;
		inline constexpr int ContainerSpacing = 18;
		inline constexpr int CopySpacing = 8;
		inline constexpr int ActionTopMargin = 2;

		inline const QSize ArtworkSize{360, MinHeight};
		inline const QSize FallbackArtworkSize{FallbackWidth, FallbackHeight};
		inline const QMargins CopyMargins{18, 14, 18, 14};
	}

	namespace Activity
	{
		inline constexpr int CollapsedHeight = 36;
		inline constexpr int ExpandedHeight = 240;
		inline constexpr int ListWidth = 280;
		inline constexpr int RowHeight = 26;
		inline constexpr int HistoryRowHeight = 34;
		inline constexpr int RunIndicatorWidth = 4;
		inline constexpr const char* ExpandGlyph = "+";
		inline constexpr const char* CollapseGlyph = "-";

		inline const QSize ToggleButtonSize{28, 24};
		inline const QMargins HeaderMargins{10, 5, 10, 5};
		inline const QMargins RailMargins{4, 4, 4, 4};
		inline const QMargins OutputMargins{6, 4, 6, 6};
	}

	namespace Section
	{
		inline constexpr int Spacing = 4;
	}

	namespace Hero
	{
		inline constexpr int DesignWidth = 1560;
		inline constexpr int DesignHeight = 360;
		inline constexpr int CopyDividerX = 624;
		inline constexpr int MinimumHeight = 260;
		inline constexpr int MinimumWidth = 720;
		inline constexpr int CopyLeft = 48;
		inline constexpr int CopyTop = 58;
		inline constexpr int CopyBottom = 44;
		inline constexpr int CopyWidth = 430;
		inline constexpr int CopySpacing = 18;
		inline constexpr int ActionTopMargin = 2;
		inline constexpr double MinimumCopyScale = 0.74;
		inline constexpr double MaximumCopyScale = 1.12;
	}

	namespace Home
	{
		inline constexpr int BodyLeft = 28;
		inline constexpr int BodyTop = 24;
		inline constexpr int BodyRight = 28;
		inline constexpr int BodyBottom = 0;
		inline constexpr int SectionSpacing = 10;
		inline constexpr int TileSpacing = 18;
		inline constexpr int ProductCardMinWidth = 420;
		inline constexpr int ProductCardMaxWidth = 720;
		inline constexpr int ProductCardMaxColumns = 3;
		inline constexpr int DiscoverCardMinWidth = (ProductCardMinWidth - TileSpacing) / 2;
		inline constexpr int DiscoverCardMaxWidth = (ProductCardMaxWidth - TileSpacing) / 2;
		inline constexpr int DiscoverCardMaxColumns = 6;
	}

	namespace Card
	{
		inline constexpr double HomeTileAspectRatio = 1.9;
		inline constexpr int ProductBodyTop = 14;
		inline constexpr int DiscoverBodyTop = 0;
		inline constexpr int ProductBodyBottom = 0;
		inline constexpr int DiscoverBodyBottom = 12;
		inline constexpr int ProductSpacing = 12;
		inline constexpr int DiscoverSpacing = 6;

		inline const QSize ProductArtworkSize{720, 180};
		inline const QSize DiscoverArtworkSize{351, 105};
		inline const QMargins FlushArtworkMargins{0, 0, 0, Space::Large};
		QMargins ProductMargins(bool hasArtwork);
		QMargins DiscoverMargins(bool hasArtwork);
		inline const QMargins ProductBodyMargins{Space::XLarge, ProductBodyTop, Space::XLarge, ProductBodyBottom};
		inline const QMargins DiscoverBodyMargins{Space::XLarge, DiscoverBodyTop, Space::XLarge, DiscoverBodyBottom};
	}
}
