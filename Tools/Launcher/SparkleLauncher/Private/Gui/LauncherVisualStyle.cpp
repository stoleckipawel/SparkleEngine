#include "LauncherVisualStyle.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>

namespace SparkleLauncher
{
	namespace
	{
		constexpr const char* kColorStateQueued = "#8b949e";
		constexpr const char* kColorStateRunning = "#76b900";
		constexpr const char* kColorStateSuccess = "#7ee787";
		constexpr const char* kColorStateDestructive = "#ff7b72";
		constexpr const char* kColorStateWarning = "#ffb454";

		void AddStyleRule(QString& style, const QString& selector, const QString& body)
		{
			style += selector + " { " + body + " }";
		}
	}

	void ApplyLauncherVisualStyle(QWidget& rootWidget)
	{
		const QString background = "#111312";
		const QString shell = "#181a19";
		const QString panel = "#202220";
		const QString panelHover = "#2c302c";
		const QString field = "#202321";
		const QString border = "#0b0d0c";
		const QString borderSoft = "#303430";
		const QString borderStrong = "#444943";
		const QString divider = "#2b2f2b";
		const QString accent = "#76b900";
		const QString accentHover = "#8bd80f";
		const QString focus = accent;
		const QString primary = accent;
		const QString primaryHover = accentHover;
		const QString selection = "#31451f";
		const QString warning = QString::fromLatin1(kColorStateWarning);
		const QString destructive = QString::fromLatin1(kColorStateDestructive);
		const QString textPrimary = "#f2f4f1";
		const QString textBody = "#d9ddd7";
		const QString textSecondary = "#b9c0b6";
		const QString textMuted = "#858d82";

		QString style;
		const auto addRule = [&style](const QString& selector, const QString& body) {
			AddStyleRule(style, selector, body);
		};

		addRule("QMainWindow, QWidget", "background: " + background + "; color: " + textBody + "; font-family: 'Segoe UI'; font-size: 9pt;");
		addRule("QLabel", "color: " + textBody + "; background: transparent;");
		addRule("#WorkflowSurface", "background: " + background + ";");
		addRule("#ProcessPanel", "background: " + shell + "; border: none; border-right: 1px solid #252923; padding: 0;");
		addRule("#OptionsPanel", "background: " + background + "; border: none;");
		addRule("#TitleBand", "background: #242622; border: none; border-bottom: 1px solid " + divider + "; min-height: 58px; max-height: 58px;");
		addRule("#HeaderUtilityPanel", "background: transparent; border: none;");
		addRule("#ActivityDrawer", "background: #181a19; border: none; border-left: 1px solid " + divider + ";");
		addRule("#OutputPanel", "background: #181a19; border: none;");
		addRule("#OutputPaneLabel", "color: " + textSecondary + "; font-size: 8pt; font-weight: 700; letter-spacing: 0.2px;");
		addRule("#ActivityRail", "background: #23262a; border: none; border-right: 1px solid " + border + ";");
		addRule("#OutputPane", "background: #202327; border: none;");
		addRule("#HeaderFieldLabel", "color: " + textMuted + "; font-size: 8pt; font-weight: 600;");
		addRule("#HeaderContextCombo", "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 2px; padding: 2px 8px; color: " + textBody + "; min-height: 24px; max-height: 28px; font-size: 8pt;");
		addRule("#HeaderContextCombo:focus", "border: 1px solid " + focus + ";");
		addRule("#HeaderUtilityButton", "background: transparent; color: " + textBody + "; border: 1px solid transparent; padding: 5px 9px; font-size: 8pt; font-weight: 750;");
		addRule("#HeaderUtilityButton:hover", "background: " + panelHover + "; color: " + textPrimary + ";");
		addRule("#HeaderUtilityButton:focus", "border: 1px solid " + focus + ";");
		addRule("#OptionsScrollArea, #OptionsStack, #OptionsContent, #OperationStack, #InlineOptionsSection, #ActivityDetailsPanel", "background: transparent; border: none;");
		addRule("#OptionsScrollArea QWidget", "background: transparent;");
		addRule("#OptionRow", "background: transparent; border-top: 1px solid " + divider + "; min-height: 32px;");
		addRule("#OptionGroup", "background: transparent; border: none; margin-top: 12px;");
		addRule("#OptionLabelCell", "background: transparent; border: none;");
		addRule("#OptionValueCell", "background: transparent; border: none;");
		addRule("#ActiveOperationLabel", "color: " + textPrimary + "; font-size: 15pt; font-weight: 800; letter-spacing: -0.15px;");
		addRule("#CommandIdentityBar", "background: transparent; border: none; padding: 2px 0 8px 0;");
		addRule("#CommandProductTitle", "color: " + textPrimary + "; font-size: 20pt; font-weight: 900; letter-spacing: -0.35px;");
		addRule("#CommandProductSubtitle", "color: " + textSecondary + "; font-size: 9pt; font-weight: 600;");
		addRule("#CommandHeroCard", "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #24291f, stop:0.55 #1d211b, stop:1 #141615); border: 1px solid #354126; border-left: 3px solid " + accent + "; border-radius: 4px;");
		addRule("#CommandHeroCard[State=\"warning\"]", "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #584129, stop:0.58 #3a3026, stop:1 #292923); border-color: #8a662f; border-top-color: #bd8939;");
		addRule("#CommandHeroCopyPane", "background: transparent; border: none;");
		addRule("#CommandHeroArtwork", "background: #050605; border: none; border-left: 1px solid #26301d;");
		addRule("#CommandHeroTitle", "color: #ffffff; font-size: 18pt; font-weight: 900; letter-spacing: -0.25px;");
		addRule("#CommandHeroText", "color: " + textBody + "; font-size: 9.5pt; line-height: 135%;");
		addRule("#CommandHeroChip", "color: #dff3cf; border: 1px solid #4d6f29; border-radius: 3px; background: #26351f; padding: 3px 9px; font-size: 7.75pt; font-weight: 800;");
		addRule("#CommandHeroChip[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
		addRule("#CommandSectionTitle", "color: " + textPrimary + "; font-size: 13pt; font-weight: 900; padding: 16px 0 4px 0; letter-spacing: -0.1px;");
		addRule("#CommandCapabilityCard", "background: " + panel + "; border: 1px solid " + divider + "; border-radius: 4px;");
		addRule("#CommandCapabilityCard[TileRole=\"library\"]", "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #252925, stop:0.62 #1e211f, stop:1 #161816); border: 1px solid #384033; border-left: 3px solid " + accent + ";");
		addRule("#CommandCapabilityCard[State=\"ok\"]", "border-left: 4px solid " + accent + ";");
		addRule("#CommandCapabilityCard[State=\"warning\"]", "border-left: 4px solid #b37726;");
		addRule("#CommandCardArtwork", "background: #070807; border: 1px solid #30372b; border-radius: 3px;");
		addRule("#CommandCardTitle", "color: " + textPrimary + "; font-size: 11.5pt; font-weight: 900; letter-spacing: -0.1px;");
		addRule("#CommandCardText", "color: " + textSecondary + "; font-size: 8.75pt; line-height: 135%;");
		addRule("#CommandCardChip", "color: " + textSecondary + "; border: 1px solid #4c5149; border-radius: 3px; background: #2b2f2a; padding: 2px 8px; font-size: 7.5pt; font-weight: 800;");
		addRule("#CommandCardChip[State=\"ok\"]", "color: #dff3cf; border-color: #4d6f29; background: #2b3522;");
		addRule("#CommandCardChip[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
		addRule("QPushButton#CommandPrimaryButton", "background: " + primary + "; color: #071006; border: 1px solid #92d83a; border-radius: 3px; padding: 7px 18px; font-weight: 900; min-width: 150px;");
		addRule("QPushButton#CommandPrimaryButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton#CommandPrimaryButton:disabled", "background: #252923; color: " + textMuted + "; border: 1px solid #3b4434;");
		addRule("QPushButton#CommandSecondaryButton", "background: #2b2f2a; color: " + textBody + "; border: 1px solid " + borderSoft + "; border-top-color: #42493f; border-radius: 3px; padding: 6px 13px; font-weight: 750; min-width: 142px;");
		addRule("QPushButton#CommandSecondaryButton:hover", "background: " + panelHover + "; color: " + textPrimary + ";");
		addRule("QPushButton#CommandSecondaryButton:disabled", "background: #252923; color: " + textMuted + "; border: 1px solid #343a33;");
		addRule("#SectionLabel", "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 800; padding: 6px 0 1px 0; letter-spacing: 0.35px;");
		addRule("#OptionGroupTitle", "color: " + textPrimary + "; font-size: 8.75pt; font-weight: 800; padding: 0 0 3px 0;");
		addRule("#DetailsToggleButton", "background: transparent; color: " + textPrimary + "; border: none; padding: 0 0 3px 0; text-align: left; font-size: 8.5pt; font-weight: 700;");
		addRule("#DetailsToggleButton:hover", "color: #ffffff;");
		addRule("#DetailsPanel", "background: transparent; border: none;");
		addRule("#FieldLabel", "color: #c9ced4; font-size: 8pt; font-weight: 600; padding-top: 0;");
		addRule("#OptionHelpText", "color: " + textMuted + "; font-size: 7.5pt; line-height: 120%; padding: 0 0 3px 0;");
		addRule("#CleanPlanText", "color: " + textSecondary + "; background: #1d201d; border-top: 1px solid " + divider + "; padding: 8px 10px; font-size: 8pt;");
		addRule("#CleanScopeCard", "background: #1d201d; border: 1px solid " + divider + "; border-left: 3px solid #4a515a; border-radius: 3px;");
		addRule("#CleanScopeCard:hover", "background: #222621; border-left-color: " + accent + ";");
		addRule("#ActionMetaPanel", "background: transparent; border: none; border-top: 1px solid " + divider + ";");
		addRule("#ActionMetaTitle", "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 700;");
		addRule("#ActionMetaText", "color: " + textBody + "; font-size: 7.75pt;");
		addRule("#ActionMetaDetail", "color: " + textMuted + "; font-size: 7.5pt;");
		addRule("#StatusRow", "background: #1d201d; border: none; border-top: 1px solid " + divider + "; padding: 9px 10px 9px 10px; margin-top: 0;");
		addRule("#StatusLabel", "color: " + textBody + "; font-size: 8.5pt; font-weight: 700;");
		addRule("#StatusValue", "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 800; padding: 2px 8px; border: 1px solid #4c5149; background: #2b2f2a; min-width: 58px;");
		addRule("#StatusValue[State=\"ok\"]", "color: #dff3cf; border-color: #4d6f29; background: #2b3522;");
		addRule("#StatusValue[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
		addRule("#StatusValue[State=\"bad\"]", "color: #ffd0cc; border-color: #79413d; background: #3a2928;");
		addRule("#StatusValue[State=\"neutral\"]", "color: " + textSecondary + "; border-color: #4c5149; background: #2b2f2a;");
		addRule("#StatusActionCell", "background: transparent; border: none;");
		addRule("#StatusDetail", "color: " + textMuted + "; font-size: 7.75pt;");
		addRule("#ActionRow", "background: transparent; border: none; padding: 4px 0;");
		addRule("#ActionTitle", "color: " + textPrimary + "; font-size: 8.5pt; font-weight: 700;");
		addRule("#InlineActionButton", "background: #2b2f2a; color: " + textBody + "; border: 1px solid " + borderSoft + "; border-top-color: #42493f; padding: 4px 10px; min-width: 116px;");
		addRule("#InlineActionButton:hover", "background: " + panelHover + ";");
		addRule("#MutedLabel", "color: " + textMuted + "; padding: 4px 0;");
		addRule("#ProgressLabel", "color: " + textPrimary + "; font-size: 9pt; font-weight: 700;");
		addRule("#ActivitySummary", "color: " + textSecondary + "; background: transparent; font-size: 7.75pt; font-weight: 600; padding: 0 0 2px 0;");
		addRule("#WorkflowGroupButton", "background: transparent; color: " + textMuted + "; border: none; border-left: 3px solid transparent; padding: 5px 4px 5px 4px; text-align: center; font-size: 7.6pt; font-weight: 700; min-width: 76px;");
		addRule("#WorkflowGroupButton:hover", "background: #20231f; color: " + textBody + "; border-left: 3px solid #3a4234;");
		addRule("#WorkflowGroupButton[ActiveState=\"true\"]", "background: #20251d; color: " + textPrimary + "; border-left: 3px solid " + accent + ";");
		addRule("#WorkflowGroupButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("#WorkflowButton", "background: transparent; color: " + textSecondary + "; border: none; border-bottom: 3px solid transparent; padding: 10px 16px 8px 16px; text-align: center; font-size: 9pt; font-weight: 750;");
		addRule("#WorkflowButton:hover", "background: #1b1e1b; color: " + textPrimary + ";");
		addRule("#WorkflowButton:checked", "background: transparent; border-bottom: 3px solid " + accent + "; color: #ffffff;");
		addRule("#WorkflowButton:focus", "border: 1px solid " + focus + "; color: " + textPrimary + ";");
		addRule("#SourceTierCard", "background: " + panel + "; border: 1px solid " + divider + "; border-radius: 4px; border-left: 4px solid #4a515a;");
		addRule("#SourceTierCard[State=\"ok\"]", "border-left-color: " + accent + ";");
		addRule("#SourceTierCard[State=\"warning\"]", "border-left-color: #b37726;");
		addRule("#SourceTierTitle", "color: " + textPrimary + "; font-size: 10.5pt; font-weight: 900;");
		addRule("#SourceTierText", "color: " + textSecondary + "; font-size: 8.25pt; line-height: 130%;");
		addRule("#SourceTierMeta", "color: " + textMuted + "; font-size: 7.5pt; font-weight: 750;");
		addRule("#SourceTierChip", "color: " + textSecondary + "; border: 1px solid #4c5149; border-radius: 3px; background: #2b2f2a; padding: 2px 8px; font-size: 7.5pt; font-weight: 800;");
		addRule("#SourceTierChip[State=\"ok\"]", "color: #dff3cf; border-color: #4d6f29; background: #2b3522;");
		addRule("#SourceTierChip[State=\"warning\"]", "color: #ffe2a8; border-color: #7a5a23; background: #3a3123;");
		addRule("QPushButton", "background: " + primary + "; color: #071006; border: 1px solid #92d83a; border-radius: 2px; padding: 6px 14px; font-weight: 750;");
		addRule("QPushButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton:focus", "border: 1px solid " + focus + ";");
		addRule("QPushButton:disabled", "background: #2d312d; border: 1px solid " + border + "; border-top-color: #41483e; color: " + textMuted + ";");
		addRule("#PrimaryActionButton", "background: " + primary + "; color: #071006; min-width: 112px; padding-left: 18px; padding-right: 18px; font-weight: 900;");
		addRule("#PrimaryActionButton:hover", "background: " + primaryHover + ";");
		addRule("#SecondaryButton", "background: #2a2d2a; color: " + textBody + "; border: 1px solid " + borderSoft + "; padding: 4px 10px; font-size: 8pt; font-weight: 650;");
		addRule("#DependencyActionButton", "background: transparent; color: " + textMuted + "; border: none; padding: 0; min-width: 16px; max-width: 16px; min-height: 16px; max-height: 16px;");
		addRule("#DependencyActionButton:hover", "background: #30362e; color: " + textPrimary + "; border-radius: 2px;");
		addRule("#OverflowMenu", "background: #20231f; color: " + textBody + "; border: 1px solid " + borderStrong + "; padding: 1px 0;");
		addRule("#OverflowMenu::item", "background: transparent; padding: 3px 10px 3px 8px; color: " + textBody + "; font-size: 7.75pt;");
		addRule("#OverflowMenu::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule("QComboBox, QLineEdit, QTextEdit", "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 2px; padding: 4px 8px; color: " + textBody + "; selection-background-color: " + selection + ";");
		addRule("QComboBox:focus, QLineEdit:focus, QTextEdit:focus", "border: 1px solid " + focus + ";");
		addRule("QComboBox:disabled", "background: " + shell + "; border: 1px solid " + border + "; color: " + textMuted + ";");
		addRule("QCheckBox", "spacing: 8px; padding: 0; color: " + textBody + "; font-size: 8pt;");
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
		addRule("#ActivityRunIndicator", "background: " + QString::fromLatin1(kColorStateQueued) + "; border-radius: 1px;");
		addRule("#ActivityRunIndicator[RunState=\"queued\"]", "background: " + QString::fromLatin1(kColorStateQueued) + ";");
		addRule("#ActivityRunIndicator[RunState=\"running\"]", "background: " + QString::fromLatin1(kColorStateRunning) + ";");
		addRule("#ActivityRunIndicator[RunState=\"done\"]", "background: " + QString::fromLatin1(kColorStateSuccess) + ";");
		addRule("#ActivityRunIndicator[RunState=\"failed\"]", "background: " + QString::fromLatin1(kColorStateDestructive) + ";");
		addRule("#ActivityRunTitle", "color: " + textBody + "; font-size: 8pt; font-weight: 650; padding: 0; margin: 0;");
		addRule("#ActivityRunState", "color: " + textMuted + "; font-size: 7pt; font-weight: 700; padding: 0; margin: 0;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunTitle", "color: #ffffff;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunState", "color: #dff3cf;");
		addRule("#OperationOutput", "background: transparent; border: none; border-radius: 0; padding: 2px 0 0 0; font-family: 'Cascadia Mono'; font-size: 8.25pt;");

		rootWidget.setStyleSheet(style);
	}
}
