#pragma once

#include <QtCore/QMargins>
#include <QtCore/QSize>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>

namespace SparkleLauncher::LauncherUi
{
	namespace Color
	{
		inline constexpr const char* Background = "#0f1110";
		inline constexpr const char* HeroBackground = "#030404";
		inline constexpr const char* Shell = "#151715";
		inline constexpr const char* Panel = "#1b1e1c";
		inline constexpr const char* PanelHover = "#242825";
		inline constexpr const char* Field = "#191c1a";
		inline constexpr const char* Border = "#222522";
		inline constexpr const char* BorderSoft = "#343934";
		inline constexpr const char* BorderStrong = "#454b45";
		inline constexpr const char* Divider = "#292d2a";
		inline constexpr const char* Accent = "#76b900";
		inline constexpr const char* AccentHover = "#8bd80f";
		inline constexpr const char* Selection = "#27351e";
		inline constexpr const char* TextPrimary = "#f4f6f3";
		inline constexpr const char* TextBody = "#d5d9d4";
		inline constexpr const char* TextSecondary = "#aeb5ac";
		inline constexpr const char* TextMuted = "#7e877d";
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
		inline constexpr int RailWidth = 168;
		inline constexpr int RailItemMinHeight = 48;
		inline constexpr int TabMinHeight = 36;
		inline constexpr int RailIconSize = 16;
		inline constexpr int RailTopPadding = 10;
		inline constexpr int RailBottomPadding = 12;
		inline constexpr int RailGroupSpacing = 4;
		inline constexpr int WorkflowTabSpacing = 18;
		inline constexpr int PanelHorizontalMargin = 18;
		inline constexpr int PanelVerticalMargin = 14;
	}

	namespace ContextSelector
	{
		inline constexpr int ComboHeight = 26;
		inline constexpr int RunModeComboMinWidth = 86;
		inline constexpr int RunModeComboMaxWidth = 110;
		inline constexpr int ConfigurationComboMinWidth = 140;
		inline constexpr int ConfigurationComboMaxWidth = 180;
		inline constexpr int IdeComboMinWidth = 120;
		inline constexpr int IdeComboMaxWidth = 150;
		inline constexpr int CompilerComboMinWidth = 100;
		inline constexpr int CompilerComboMaxWidth = 130;
		inline constexpr int GraphicsApiComboMinWidth = 92;
		inline constexpr int GraphicsApiComboMaxWidth = 116;
		inline constexpr int ShaderBackendComboMinWidth = 82;
		inline constexpr int ShaderBackendComboMaxWidth = 108;
	}

	namespace FooterContext
	{
		inline constexpr int ItemSpacing = 10;
		inline constexpr int FieldSpacing = 0;
		inline constexpr int DividerHeight = 32;
		inline const QMargins Margins{18, 6, 18, 7};
		inline const QMargins FieldMargins{0, 0, 0, 0};
	}

	namespace Selector
	{
		inline constexpr int PopupHorizontalPadding = 36;
		inline constexpr int PopupMaxWidth = 440;
	}

	namespace Row
	{
		inline constexpr int FieldLabelWidth = 132;
		inline constexpr int FieldValueWidth = 560;
		inline constexpr int InlineStatusWidth = 88;
		inline constexpr int StatusActionWidth = 88;
		inline constexpr int StatusActionHeight = 28;
	}

	namespace Button
	{
		inline constexpr int PrimaryMinHeight = 36;
		inline constexpr int SecondaryMinHeight = 32;
	}

	namespace Page
	{
		inline constexpr int MaxContentWidth = 1280;
		inline constexpr int Spacing = 12;
		inline constexpr int BottomMargin = 32;
		inline const QMargins ContentMargins{36, 28, 36, BottomMargin};
		inline const QMargins QuickStartMargins{0, 0, 0, BottomMargin};
	}

	namespace ActionMeta
	{
		inline constexpr int Spacing = 10;
		inline constexpr int PrimaryButtonWidth = 136;
		inline constexpr int SecondaryButtonWidth = 88;
		inline constexpr int ButtonHeight = 36;
		inline const QMargins Margins{Page::ContentMargins.left(), 8, Page::ContentMargins.right(), 8};
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
		inline constexpr int RowSpacing = 14;
		inline const QMargins RowMargins{16, 11, 16, 11};
	}

	namespace ScopeSelection
	{
		inline constexpr int ContentMinWidth = 760;
		inline constexpr int ContentMaxWidth = 1208;
		inline constexpr int RowSpacing = 14;
		inline const QMargins RowMargins{16, 11, 16, 11};
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
		inline constexpr int CollapsedHeight = 40;
		inline constexpr int ExpandedHeight = 260;
		inline constexpr int ListWidth = 280;
		inline constexpr int RowHeight = 26;
		inline constexpr int HistoryRowHeight = 34;
		inline constexpr int RunIndicatorWidth = 4;
		inline constexpr const char* ExpandGlyph = "Show";
		inline constexpr const char* CollapseGlyph = "Hide";

		inline const QSize ToggleButtonSize{50, 24};
		inline const QMargins HeaderMargins{18, 7, 18, 7};
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
		inline constexpr int DesignHeight = 320;
		inline constexpr int CopyDividerX = 624;
		inline constexpr int MinimumHeight = 238;
		inline constexpr int MinimumWidth = 720;
		inline constexpr int CopyLeft = 56;
		inline constexpr int CopyTop = 48;
		inline constexpr int CopyBottom = 36;
		inline constexpr int CopyWidth = 450;
		inline constexpr int CopySpacing = 14;
		inline constexpr int ActionTopMargin = 2;
		inline constexpr double MinimumCopyScale = 0.74;
		inline constexpr double MaximumCopyScale = 1.12;
	}

	namespace Home
	{
		inline constexpr int BodyLeft = 36;
		inline constexpr int BodyTop = 26;
		inline constexpr int BodyRight = 36;
		inline constexpr int BodyBottom = 0;
		inline constexpr int SectionSpacing = 10;
	}

	namespace MapCatalog
	{
		inline constexpr int MinimumCardWidth = 400;
		inline constexpr int MaximumCardWidth = 620;
		inline constexpr int MaximumColumns = 4;
		inline constexpr int HorizontalSpacing = 14;
		inline constexpr int VerticalSpacing = 14;
		inline constexpr int CardContentSpacing = 7;
		inline constexpr double CardAspectRatio = 2.42;
		inline const QMargins CardBodyMargins{14, 11, 14, 11};
	}
}
