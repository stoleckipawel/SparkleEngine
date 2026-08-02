#include "LauncherVisualStyle.h"

#include "LauncherUiDesign.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>

#ifdef Q_OS_WIN
  #ifndef NOMINMAX
	#define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace SparkleLauncher
{

	void AddStyleRule(QString& style, const QString& selector, const QString& body)
	{
		style += selector + " { " + body + " }";
	}

	QString UiColor(const char* color)
	{
		return QString::fromLatin1(color);
	}

	void ApplyLauncherVisualStyle(QWidget& rootWidget)
	{
		const QString background = UiColor(LauncherUi::Color::Background);
		const QString shell = UiColor(LauncherUi::Color::Shell);
		const QString panel = UiColor(LauncherUi::Color::Panel);
		const QString panelHover = UiColor(LauncherUi::Color::PanelHover);
		const QString field = UiColor(LauncherUi::Color::Field);
		const QString border = UiColor(LauncherUi::Color::Border);
		const QString borderSoft = UiColor(LauncherUi::Color::BorderSoft);
		const QString borderStrong = UiColor(LauncherUi::Color::BorderStrong);
		const QString divider = UiColor(LauncherUi::Color::Divider);
		const QString accent = UiColor(LauncherUi::Color::Accent);
		const QString accentHover = UiColor(LauncherUi::Color::AccentHover);
		const QString focus = accent;
		const QString primary = accent;
		const QString primaryHover = accentHover;
		const QString selection = UiColor(LauncherUi::Color::Selection);
		const QString warning = UiColor(LauncherUi::Color::StateWarning);
		const QString destructive = UiColor(LauncherUi::Color::StateDestructive);
		const QString textPrimary = UiColor(LauncherUi::Color::TextPrimary);
		const QString textBody = UiColor(LauncherUi::Color::TextBody);
		const QString textSecondary = UiColor(LauncherUi::Color::TextSecondary);
		const QString textMuted = UiColor(LauncherUi::Color::TextMuted);

		QString style;
		const auto addRule = [&style](const QString& selector, const QString& body)
		{
			AddStyleRule(style, selector, body);
		};
		const auto addStateChipRules = [&](const QString& selector, const QString& extra = QString())
		{
			addRule(
			    selector,
			    "color: " + textSecondary
			        + "; border: 1px solid #4c5149; border-radius: 3px; background: #2b2f2a; "
			          "padding: 2px 8px; font-size: 7.5pt; font-weight: 800;"
			        + extra);
			addRule(selector + "[State=\"ok\"]", "color: #dff3cf; border-color: #4d6f29; background: #2b3522;");
			addRule(selector + "[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
			addRule(selector + "[State=\"bad\"]", "color: #ffd0cc; border-color: #79413d; background: #3a2928;");
			addRule(selector + "[State=\"neutral\"]", "color: " + textSecondary + "; border-color: #4c5149; background: #2b2f2a;");
		};

		addRule(
		    "QMainWindow, QWidget",
		    "background: " + background + "; color: " + textBody + "; font-family: 'Segoe UI'; font-size: 9.25pt;");
		addRule("QLabel", "color: " + textBody + "; background: transparent;");
		addRule("#WorkflowSurface", "background: " + background + ";");
		addRule("#ProcessPanel", "background: " + shell + "; border: none; border-right: 1px solid #252923; padding: 0;");
		addRule("#OptionsPanel", "background: " + background + "; border: none;");
		addRule(
		    "#TitleBand",
		    "background: #242622; border: none; border-bottom: 1px solid " + divider + "; min-height: 58px; max-height: 58px;");
		addRule("#HeaderUtilityPanel", "background: transparent; border: none;");
		addRule("#ActivityBottomPanel", "background: #181a19; border: none; border-top: 1px solid " + divider + ";");
		addRule("#OutputPanel", "background: #181a19; border: none;");
		addRule("#ActivityHeader", "background: #181a19; border: none;");
		addRule(
		    "#ActivityToggleButton",
		    "background: transparent; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; border-radius: 2px; padding: 0; font-size: 10pt; font-weight: 900; min-width: 28px; max-width: 28px; min-height: "
		          "24px; max-height: 24px;");
		addRule(
		    "#ActivityToggleButton:hover",
		    "background: " + panelHover + "; color: " + textPrimary + "; border-color: " + borderStrong + ";");
		addRule("#ActivityToggleButton:focus", "border: 1px solid " + focus + ";");
		addRule("#OutputPaneLabel", "color: " + textSecondary + "; font-size: 8pt; font-weight: 700; letter-spacing: 0.2px;");
		addRule(
		    "#ActivityRail",
		    "background: #23262a; border: none; border-top: 1px solid " + divider + "; border-right: 1px solid " + border + ";");
		addRule("#OutputPane", "background: #202327; border: none; border-top: 1px solid " + divider + ";");
		addRule("#HeaderFieldLabel", "color: " + textMuted + "; font-size: 8.25pt; font-weight: 650;");
		addRule(
		    "#HeaderContextCombo",
		    "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 2px; padding: 3px 9px; color: " + textBody
		        + "; min-height: 26px; max-height: 30px; font-size: 8.5pt;");
		addRule("#HeaderContextCombo:focus", "border: 1px solid " + focus + ";");
		addRule(
		    "#HeaderUtilityButton",
		    "background: transparent; color: " + textBody
		        + "; border: 1px solid transparent; padding: 5px 9px; font-size: 8pt; font-weight: 750;");
		addRule("#HeaderUtilityButton:hover", "background: " + panelHover + "; color: " + textPrimary + ";");
		addRule("#HeaderUtilityButton:focus", "border: 1px solid " + focus + ";");
		addRule(
		    "#OptionsScrollArea, #OptionsStack, #OptionsContent, #OperationStack, #InlineOptionsSection, #ActivityDetailsPanel",
		    "background: transparent; border: none;");
		addRule("#OptionsScrollArea QWidget", "background: transparent;");
		addRule("#OptionRow", "background: transparent; border-top: 1px solid " + divider + "; min-height: 38px;");
		addRule("#OptionGroup", "background: transparent; border: none; margin-top: 14px;");
		addRule("#OptionLabelCell", "background: transparent; border: none;");
		addRule("#OptionValueCell", "background: transparent; border: none;");
		addRule("#ActiveOperationLabel", "color: " + textPrimary + "; font-size: 15.5pt; font-weight: 850; letter-spacing: -0.18px;");
		addRule("#CommandHeroCard", "background: transparent; border: none; border-radius: 0;");
		addRule("#CommandHeroCard[State=\"warning\"]", "background: transparent; border: none;");
		addRule("#CommandHeroOverlay", "background: transparent; border: none;");
		addRule("#CommandHeroCopyPane", "background: transparent; border: none;");
		addRule("#CommandHeroArtwork", "background: transparent; border: none;");
		addRule("#CommandHeroTitle", "color: #ffffff; font-size: 20pt; font-weight: 900; letter-spacing: -0.3px;");
		addRule("#CommandHeroText", "color: " + textBody + "; font-size: 10pt; line-height: 142%;");
		addRule(
		    "#CommandSectionTitle",
		    "color: " + textPrimary + "; font-size: 12.5pt; font-weight: 900; padding: 18px 0 6px 0; letter-spacing: -0.1px;");
		addRule("#CommandCapabilityCard", "background: " + panel + "; border: 1px solid " + divider + "; border-radius: 4px;");
		addRule(
		    "#CommandCapabilityCard[TileRole=\"library\"]",
		    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #252925, stop:0.62 #1e211f, stop:1 #161816); border: 1px solid "
		    "#384033; border-left: 3px solid "
		        + accent + ";");
		addRule(
		    "#CommandCapabilityCard[TileRole=\"discover\"]",
		    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #20231f, stop:0.62 #191c19, stop:1 #121512); border: 1px solid "
		    "#30372b; border-left: 3px solid #b37726;");
		addRule(
		    "#CommandCapabilityCard[TileRole=\"discover\"][Interactive=\"true\"]:hover",
		    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #252a22, stop:0.62 #1b2019, stop:1 #121512); border-color: #4b573f;");
		addRule("#CommandCapabilityCard[State=\"ok\"]", "border-left: 4px solid " + accent + ";");
		addRule("#CommandCapabilityCard[State=\"warning\"]", "border-left: 4px solid #b37726;");
		addRule("#CommandCardArtwork", "background: #070807; border: 1px solid #30372b; border-radius: 3px;");
		addRule("#CommandCardArtwork[TileRole=\"library\"]", "background: #070807; border: none; border-radius: 0;");
		addRule("#CommandCardArtwork[TileRole=\"discover\"]", "background: #070807; border: none; border-radius: 2px;");
		addRule("#CommandCardTitle", "color: " + textPrimary + "; font-size: 11.25pt; font-weight: 900; letter-spacing: -0.08px;");
		addRule("#CommandCapabilityCard[TileRole=\"library\"] #CommandCardTitle", "font-size: 12.5pt; letter-spacing: -0.12px;");
		addRule("#CommandCapabilityCard[TileRole=\"discover\"] #CommandCardTitle", "font-size: 10.5pt; letter-spacing: -0.06px;");
		addRule("#CommandCardText", "color: " + textSecondary + "; font-size: 9pt; line-height: 135%;");
		addRule("#CommandCapabilityCard[TileRole=\"discover\"] #CommandCardText", "font-size: 8.35pt; line-height: 128%;");
		addStateChipRules("#CommandCardChip");
		addRule(
		    "QPushButton#CommandPrimaryButton",
		    "background: " + primary
		        + "; color: #071006; border: 1px solid #92d83a; border-radius: 3px; padding: 7px 18px; font-weight: 900; min-width: "
		          "150px;");
		addRule("QPushButton#CommandPrimaryButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton#CommandPrimaryButton:disabled", "background: #252923; color: " + textMuted + "; border: 1px solid #3b4434;");
		addRule(
		    "QPushButton#CommandSecondaryButton",
		    "background: #2b2f2a; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; border-top-color: #42493f; border-radius: 3px; padding: 6px 13px; font-weight: 750; min-width: 142px;");
		addRule("QPushButton#CommandSecondaryButton:hover", "background: " + panelHover + "; color: " + textPrimary + ";");
		addRule("QPushButton#CommandSecondaryButton:disabled", "background: #252923; color: " + textMuted + "; border: 1px solid #343a33;");
		addRule(
		    "#SectionLabel",
		    "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 800; padding: 6px 0 1px 0; letter-spacing: 0.35px;");
		addRule("#OptionGroupTitle", "color: " + textPrimary + "; font-size: 9.25pt; font-weight: 850; padding: 0 0 4px 0;");
		addRule(
		    "#DetailsToggleButton",
		    "background: transparent; color: " + textPrimary
		        + "; border: none; padding: 0 0 4px 0; text-align: left; font-size: 9pt; font-weight: 750;");
		addRule("#DetailsToggleButton:hover", "color: #ffffff;");
		addRule("#DetailsPanel", "background: transparent; border: none;");
		addRule("#FieldLabel", "color: #c9ced4; font-size: 8.5pt; font-weight: 650; padding-top: 0;");
		addRule("#OptionHelpText", "color: " + textMuted + "; font-size: 8pt; line-height: 125%; padding: 0 0 5px 0;");
		addRule(
		    "#CleanPlanText",
		    "color: " + textSecondary + "; background: #1d201d; border-top: 1px solid " + divider + "; padding: 8px 10px; font-size: 8pt;");
		addRule("#CleanSelectionSummary", "color: " + textBody + "; font-size: 8.25pt; font-weight: 750; padding: 1px 0 7px 0;");
		addRule("#CleanSelectionPanel", "background: #171a18; border: 1px solid " + divider + "; border-radius: 3px;");
		addRule(
		    "#CleanScopeGroupTitle",
		    "background: #1d211d; color: " + textSecondary + "; border: none; border-bottom: 1px solid " + divider
		        + "; padding: 7px 13px 5px 13px; font-size: 7.5pt; font-weight: 850;");
		addRule("#CleanScopeRow", "background: transparent; border: none; border-bottom: 1px solid " + divider + ";");
		addRule("#CleanScopeRow:hover", "background: #1d211d;");
		addRule("#CleanScopeRow[Selected=\"true\"]", "background: #20261d;");
		addRule("#CleanScopeCheckBox", "color: " + textPrimary + "; font-size: 8.5pt; font-weight: 800;");
		addRule("#CleanScopeDescription", "color: " + textSecondary + "; font-size: 7.7pt;");
		addRule("#CleanScopePreview", "color: " + textMuted + "; font-size: 7.5pt; padding-left: 12px;");
		addRule("#ActionMetaPanel", "background: transparent; border: none; border-top: 1px solid " + divider + ";");
		addRule("#ActionMetaTitle", "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 700;");
		addRule("#ActionMetaText", "color: " + textBody + "; font-size: 7.75pt;");
		addRule("#ActionMetaDetail", "color: " + textMuted + "; font-size: 7.5pt;");
		addRule(
		    "#StatusRow",
		    "background: #1d201d; border: none; border-top: 1px solid " + divider + "; padding: 10px 10px 10px 10px; margin-top: 0;");
		addRule("#StatusLabel", "color: " + textBody + "; font-size: 8.75pt; font-weight: 750;");
		addStateChipRules("#StatusValue", " font-size: 7.75pt; font-weight: 850; min-width: 58px;");
		addRule("#StatusActionCell", "background: transparent; border: none;");
		addRule("#StatusDetail", "color: " + textMuted + "; font-size: 8pt;");
		addRule("#MapCatalogCard", "background: #2b2e32; border: 1px solid #3c4147; border-radius: 2px;");
		addRule("#MapCatalogCard:hover", "background: #30343a; border-color: #59616a;");
		addRule("#MapCatalogCard[State=\"ok\"]", "border-color: #587d31;");
		addRule("#MapCatalogCard[State=\"warning\"]", "border-color: #7a5a23;");
		addRule("#MapCardThumbnail", "background: #0b0d0c; border: none; border-right: 1px solid #414750;");
		addRule("#MapCardBody", "background: #2b2e32; border: none;");
		addRule("#MapCardTitle", "background: " + selection + "; color: #f4f7f1; font-size: 10.5pt; font-weight: 800; padding: 4px 7px;");
		addRule("#MapCardDescription", "color: #c8ccd1; font-size: 8.6pt; line-height: 130%; padding: 1px 2px;");
		addRule("#MapCardMeta", "color: " + textMuted + "; font-size: 7.35pt; padding: 0 2px;");
		addRule(
		    "#MapCardSourceButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: 1px solid #555c64; min-height: 28px; padding: 4px 11px; font-size: 8pt; font-weight: 700;");
		addRule("#MapCardSourceButton:hover", "background: #343a34; color: #ffffff; border-color: #697866;");
		addRule("#MapCardActionButton", "min-width: 126px; min-height: 34px; padding: 6px 20px; font-size: 9pt; font-weight: 900;");
		addRule("#MapCardActionButton[ActionState=\"sync\"]", "background: " + accent + "; color: #071006; border: 1px solid #92d83a;");
		addRule("#MapCardActionButton[ActionState=\"sync\"]:hover", "background: " + accentHover + "; border-color: #a8ed4f;");
		addRule("#MapCardActionButton[ActionState=\"clean\"]", "background: #30362e; color: " + textBody + "; border: 1px solid #66715f;");
		addRule("#MapCardActionButton[ActionState=\"clean\"]:hover", "background: #3a4237; border-color: #83917a;");
		addRule("#MapCardActionButton:disabled", "background: #2d312d; color: " + textMuted + "; border-color: #41483e;");
		addRule(
		    "#MapCardActionButton[ActionState=\"sync\"]:disabled",
		    "background: " + accent + "; color: #071006; border: 1px solid #92d83a;");
		addRule("#OptionsScrollArea #MapCatalogCard", "background: #2b2e32; border: 1px solid #3c4147;");
		addRule("#OptionsScrollArea #MapCardBody", "background: #2b2e32; border: none;");
		addRule("#OptionsScrollArea #MapCardTitle", "background: " + selection + "; color: #f4f7f1; padding: 4px 7px;");
		addRule("#ActionRow", "background: transparent; border: none; padding: 4px 0;");
		addRule("#ActionTitle", "color: " + textPrimary + "; font-size: 8.5pt; font-weight: 700;");
		addRule(
		    "#InlineActionButton",
		    "background: #2b2f2a; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; border-top-color: #42493f; padding: 4px 10px; min-width: 116px;");
		addRule("#InlineActionButton:hover", "background: " + panelHover + ";");
		addRule("#MutedLabel", "color: " + textMuted + "; padding: 4px 0;");
		addRule("#ProgressLabel", "color: " + textPrimary + "; font-size: 9pt; font-weight: 700;");
		addRule(
		    "#ActivitySummary",
		    "color: " + textSecondary + "; background: transparent; font-size: 7.75pt; font-weight: 600; padding: 0 0 2px 0;");
		addRule(
		    "#WorkflowGroupButton",
		    "background: transparent; color: " + textMuted
		        + "; border: none; border-left: 3px solid transparent; padding: 5px 5px 5px 5px; text-align: center; font-size: 7.8pt; "
		          "font-weight: 750; min-width: 82px;");
		addRule("#WorkflowGroupButton:hover", "background: #20231f; color: " + textBody + "; border-left: 3px solid #3a4234;");
		addRule(
		    "#WorkflowGroupButton[ActiveState=\"true\"]",
		    "background: #20251d; color: " + textPrimary + "; border-left: 3px solid " + accent + ";");
		addRule("#WorkflowGroupButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule(
		    "#WorkflowButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: none; border-bottom: 3px solid transparent; padding: 11px 17px 9px 17px; text-align: center; font-size: "
		          "9.25pt; font-weight: 800;");
		addRule("#WorkflowButton:hover", "background: #1b1e1b; color: " + textPrimary + ";");
		addRule("#WorkflowButton:checked", "background: transparent; border-bottom: 3px solid " + accent + "; color: #ffffff;");
		addRule("#WorkflowButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule(
		    "#SourceTierCard",
		    "background: " + panel + "; border: 1px solid " + divider + "; border-radius: 4px; border-left: 4px solid #4a515a;");
		addRule("#SourceTierCard[State=\"ok\"]", "border-left-color: " + accent + ";");
		addRule("#SourceTierCard[State=\"warning\"]", "border-left-color: #b37726;");
		addRule("#SourceTierTitle", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 900;");
		addRule("#SourceTierText", "color: " + textSecondary + "; font-size: 8.25pt; line-height: 130%;");
		addRule("#SourceTierMeta", "color: " + textMuted + "; font-size: 7.5pt; font-weight: 750;");
		addStateChipRules("#SourceTierChip");
		addRule(
		    "QPushButton",
		    "background: " + primary
		        + "; color: #071006; border: 1px solid #92d83a; border-radius: 2px; padding: 6px 14px; font-weight: 750;");
		addRule("QPushButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton:focus", "border: 1px solid " + focus + ";");
		addRule(
		    "QPushButton:disabled",
		    "background: #2d312d; border: 1px solid " + border + "; border-top-color: #41483e; color: " + textMuted + ";");
		addRule(
		    "#PrimaryActionButton",
		    "background: " + primary + "; color: #071006; min-width: 112px; padding-left: 18px; padding-right: 18px; font-weight: 900;");
		addRule("#PrimaryActionButton:hover", "background: " + primaryHover + ";");
		addRule(
		    "#SecondaryButton",
		    "background: #2a2d2a; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; padding: 4px 10px; font-size: 8pt; font-weight: 650;");
		addRule(
		    "#DependencyActionButton",
		    "background: transparent; color: " + textMuted
		        + "; border: none; padding: 0; min-width: 16px; max-width: 16px; min-height: 16px; max-height: 16px;");
		addRule("#DependencyActionButton:hover", "background: #30362e; color: " + textPrimary + "; border-radius: 2px;");
		addRule("#OverflowMenu", "background: #20231f; color: " + textBody + "; border: 1px solid " + borderStrong + "; padding: 1px 0;");
		addRule("#OverflowMenu::item", "background: transparent; padding: 3px 10px 3px 8px; color: " + textBody + "; font-size: 7.75pt;");
		addRule("#OverflowMenu::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule(
		    "QComboBox, QLineEdit, QTextEdit",
		    "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 2px; padding: 5px 9px; color: " + textBody
		        + "; selection-background-color: " + selection + "; min-height: 26px;");
		addRule("QComboBox:focus, QLineEdit:focus, QTextEdit:focus", "border: 1px solid " + focus + ";");
		addRule("QComboBox:disabled", "background: " + shell + "; border: 1px solid " + border + "; color: " + textMuted + ";");
		addRule("QCheckBox", "spacing: 8px; padding: 0; color: " + textBody + "; font-size: 8.5pt;");
		addRule("QCheckBox:focus", "border: 1px solid " + focus + "; border-radius: 2px; color: " + textPrimary + ";");
		addRule("QCheckBox:disabled", "color: " + textMuted + ";");
		addRule("#WarningCheckBox", "color: " + warning + ";");
		addRule("#DestructiveCheckBox", "color: " + destructive + ";");
		addRule("QListWidget", "background: transparent; border: none; border-radius: 0; padding: 0; outline: 0;");
		addRule("QListWidget::item", "padding: 3px 4px; border-radius: 0; color: " + textBody + ";");
		addRule("QListWidget::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule("QScrollBar:vertical", "background: #151713; width: 10px; margin: 0;");
		addRule("QScrollBar::handle:vertical", "background: #3a4037; border-radius: 4px; min-height: 36px;");
		addRule("QScrollBar::handle:vertical:hover", "background: #58614f;");
		addRule("QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical", "height: 0; background: transparent;");
		addRule("QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical", "background: transparent;");
		addRule("#ActivityDetailsPanel", "background: transparent; border: none;");
		addRule("#ActivityList", "background: transparent; border: none; border-radius: 0; padding: 0;");
		addRule("#ActivityRunRow", "background: transparent; border: 1px solid transparent; padding: 1px 0;");
		addRule("#ActivityRunRow[Selected=\"true\"]", "background: " + selection + "; border: 1px solid #5c8c22; border-radius: 2px;");
		addRule("#ActivityRunIndicator", "background: " + UiColor(LauncherUi::Color::StateQueued) + "; border-radius: 1px;");
		addRule("#ActivityRunIndicator[RunState=\"queued\"]", "background: " + UiColor(LauncherUi::Color::StateQueued) + ";");
		addRule("#ActivityRunIndicator[RunState=\"running\"]", "background: " + UiColor(LauncherUi::Color::StateRunning) + ";");
		addRule("#ActivityRunIndicator[RunState=\"done\"]", "background: " + UiColor(LauncherUi::Color::StateSuccess) + ";");
		addRule("#ActivityRunIndicator[RunState=\"failed\"]", "background: " + UiColor(LauncherUi::Color::StateDestructive) + ";");
		addRule("#ActivityRunTitle", "color: " + textBody + "; font-size: 8pt; font-weight: 650; padding: 0; margin: 0;");
		addRule("#ActivityRunState", "color: " + textMuted + "; font-size: 7pt; font-weight: 700; padding: 0; margin: 0;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunTitle", "color: #ffffff;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunState", "color: #dff3cf;");
		addRule(
		    "#OperationOutput",
		    "background: transparent; border: none; border-radius: 0; padding: 2px 0 0 0; font-family: 'Cascadia Mono'; font-size: "
		    "8.25pt;");

		rootWidget.setStyleSheet(style);
	}

	void ApplyNativeDarkTitleBar(QWidget& window)
	{
#ifdef Q_OS_WIN
		using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
		HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
		if (dwmapi == nullptr)
		{
			return;
		}

		auto setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
		if (setWindowAttribute == nullptr)
		{
			FreeLibrary(dwmapi);
			return;
		}

		HWND hwnd = reinterpret_cast<HWND>(window.winId());
		BOOL darkMode = TRUE;
		constexpr DWORD kDwmUseImmersiveDarkMode = 20;
		constexpr DWORD kDwmUseImmersiveDarkModeLegacy = 19;
		HRESULT result = setWindowAttribute(hwnd, kDwmUseImmersiveDarkMode, &darkMode, sizeof(darkMode));
		if (FAILED(result))
		{
			setWindowAttribute(hwnd, kDwmUseImmersiveDarkModeLegacy, &darkMode, sizeof(darkMode));
		}

		constexpr DWORD kDwmCaptionColor = 35;
		constexpr DWORD kDwmTextColor = 36;
		const COLORREF captionColor = RGB(17, 19, 18);
		const COLORREF textColor = RGB(242, 244, 241);
		setWindowAttribute(hwnd, kDwmCaptionColor, &captionColor, sizeof(captionColor));
		setWindowAttribute(hwnd, kDwmTextColor, &textColor, sizeof(textColor));
		FreeLibrary(dwmapi);
#else
		Q_UNUSED(window);
#endif
	}
}
