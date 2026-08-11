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
			addRule(selector + "[State=\"running\"]", "color: #d8c996; border-color: #736438; background: #3c351f;");
			addRule(selector + "[State=\"bad\"]", "color: #ffd0cc; border-color: #79413d; background: #3a2928;");
			addRule(selector + "[State=\"neutral\"]", "color: " + textSecondary + "; border-color: #4c5149; background: #2b2f2a;");
		};

		addRule(
		    "QMainWindow, QWidget",
		    "background: " + background + "; color: " + textBody + "; font-family: 'Segoe UI Variable', 'Segoe UI'; font-size: 9.5pt;");
		addRule("QLabel", "color: " + textBody + "; background: transparent;");
		addRule("#WorkflowSurface", "background: " + background + ";");
		addRule("#ProcessPanel", "background: " + shell + "; border: none; border-right: 1px solid " + divider + "; padding: 0;");
		addRule("#OptionsPanel", "background: " + background + "; border: none;");
		addRule("#FooterContextPanel", "background: #181b19; border: none; border-top: 1px solid " + divider + ";");
		addRule("#ActivityBottomPanel", "background: #171917; border: none;");
		addRule("#OutputPanel", "background: #171917; border: none;");
		addRule("#ActivityHeader", "background: #191c1a; border: none; border-top: 1px solid " + divider + ";");
		addRule("#ActivityHeader #OutputPaneLabel", "color: " + textBody + "; font-size: 9pt; font-weight: 700;");
		addRule(
		    "#ActivityToggleButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: 1px solid #414740; border-radius: 3px; padding: 0; font-size: "
		          "8pt; font-weight: 700; min-width: 50px; max-width: 50px; min-height: 24px; max-height: 24px;");
		addRule(
		    "#ActivityToggleButton:hover",
		    "background: " + panelHover + "; color: " + textPrimary + "; border-color: " + borderStrong + ";");
		addRule("#ActivityToggleButton:focus", "border: 1px solid " + focus + ";");
		addRule("#OutputPaneLabel", "color: " + textSecondary + "; font-size: 8.5pt; font-weight: 700; letter-spacing: 0.2px;");
		addRule(
		    "#ActivityRail",
		    "background: #1b1e1c; border: none; border-top: 1px solid " + divider + "; border-right: 1px solid " + border + ";");
		addRule("#OutputPane", "background: #181b19; border: none; border-top: 1px solid " + divider + ";");
		addRule("#FooterContextItem", "background: transparent; border: none;");
		addRule("#FooterContextDivider", "background: #363b37; border: none;");
		addRule("#FooterFieldLabel", "color: " + textMuted + "; font-size: 8pt; font-weight: 650; padding: 0 4px;");
		addRule(
		    "#FooterContextCombo",
		    "background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 1px 22px 1px 3px; color: " + textPrimary
		        + "; min-height: 24px; max-height: 26px; font-size: 9pt; font-weight: 700;");
		addRule("#FooterContextCombo:hover", "background: #242825; border-color: #414741;");
		addRule("#FooterContextCombo:focus, #FooterContextCombo:on", "background: #222820; border-color: " + focus + ";");
		addRule("#FooterContextCombo:disabled", "background: transparent; border-color: transparent; color: " + textMuted + ";");
		addRule("#FooterContextCombo::drop-down", "border: none; width: 18px;");
		addRule(
		    "#FooterContextCombo::down-arrow",
		    "image: url(:/SparkleLauncher/footer-chevron.xpm); width: 7px; height: 4px; margin-right: 4px;");
		addRule(
		    "#OptionsScrollArea, #OptionsStack, #OptionsContent, #OperationStack, #InlineOptionsSection, #ActivityDetailsPanel",
		    "background: transparent; border: none;");
		addRule("#OptionsScrollArea QWidget", "background: transparent;");
		addRule("#OptionRow", "background: transparent; border-top: 1px solid " + divider + "; min-height: 38px;");
		addRule("#OptionGroup", "background: transparent; border: none; margin-top: 4px;");
		addRule("#OptionLabelCell", "background: transparent; border: none;");
		addRule("#OptionValueCell", "background: transparent; border: none;");
		addRule("#CommandHeroCard", "background: transparent; border: none; border-radius: 0;");
		addRule("#CommandHeroCard[State=\"warning\"]", "background: transparent; border: none;");
		addRule("#CommandHeroOverlay", "background: transparent; border: none;");
		addRule("#CommandHeroCopyPane", "background: transparent; border: none;");
		addRule("#CommandHeroArtwork", "background: transparent; border: none;");
		addRule("#CommandHeroTitle", "color: #ffffff; font-size: 24pt; font-weight: 700; letter-spacing: -0.35px;");
		addRule("#CommandHeroText", "color: " + textBody + "; font-size: 10.25pt; line-height: 140%;");
		addRule(
		    "#CommandSectionTitle",
		    "color: " + textPrimary + "; font-size: 13pt; font-weight: 700; padding: 18px 0 6px 0; letter-spacing: -0.1px;");
		addRule(
		    "#CommandCapabilityCard",
		    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #252925, stop:0.62 #1e211f, stop:1 #161816); border: 1px solid "
		    "#384033; border-left: 3px solid "
		        + accent + "; border-radius: 4px;");
		addRule("#CommandCardArtwork", "background: #070807; border: none; border-radius: 0;");
		addRule("#CommandCardTitle", "color: " + textPrimary + "; font-size: 12.5pt; font-weight: 700; letter-spacing: -0.12px;");
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
		    "color: " + textSecondary + "; font-size: 8.5pt; font-weight: 700; padding: 8px 0 3px 0; letter-spacing: 0.2px;");
		addRule("#OptionGroupTitle", "color: " + textPrimary + "; font-size: 13.5pt; font-weight: 700; padding: 0 0 3px 0;");
		addRule("#FieldLabel", "color: " + textSecondary + "; font-size: 9pt; font-weight: 650; padding-top: 0;");
		addRule("#OptionHelpText", "color: " + textSecondary + "; font-size: 9pt; line-height: 132%; padding: 0 0 9px 0;");
		addRule(
		    "#CleanPlanText",
		    "color: " + textSecondary + "; background: #1d201d; border-top: 1px solid " + divider + "; padding: 8px 10px; font-size: 8pt;");
		addRule("#CleanSelectionSummary", "color: " + textBody + "; font-size: 8.25pt; font-weight: 750; padding: 1px 0 7px 0;");
		addRule("#CleanSelectionPanel", "background: #171917; border: 1px solid " + divider + "; border-radius: 4px;");
		addRule(
		    "#CleanScopeGroupTitle",
		    "background: #1b1e1c; color: " + textSecondary + "; border: none; border-bottom: 1px solid " + divider
		        + "; padding: 8px 15px 6px 15px; font-size: 8.25pt; font-weight: 700;");
		addRule("#CleanScopeRow", "background: transparent; border: none; border-bottom: 1px solid " + divider + ";");
		addRule("#CleanScopeRow:hover", "background: #1d211e;");
		addRule("#CleanScopeRow[Selected=\"true\"]", "background: #221c1b; border-left: 2px solid #94524d;");
		addRule("#CleanScopeCheckBox", "color: " + textPrimary + "; font-size: 9.25pt; font-weight: 700;");
		addRule("#CleanScopeCheckBox::indicator:checked", "background: #b64f48; border: 1px solid #dc7067;");
		addRule("#CleanScopeDescription", "color: " + textSecondary + "; font-size: 8.5pt;");
		addRule("#CleanScopePreview", "color: " + textMuted + "; font-size: 8.25pt; padding-left: 14px;");
		addRule("#WorkflowSelectionSummary", "color: " + textBody + "; font-size: 8.75pt; font-weight: 650; padding: 1px 0 7px 0;");
		addRule("#WorkflowSelectionSummary[State=\"warning\"]", "color: " + warning + ";");
		addRule("#WorkflowSelectionPanel", "background: #171917; border: 1px solid " + divider + "; border-radius: 4px;");
		addRule("#WorkflowScopeRow", "background: transparent; border: none; border-bottom: 1px solid " + divider + ";");
		addRule("#WorkflowScopeRow:hover", "background: #1d211e;");
		addRule("#WorkflowScopeRow[Selected=\"true\"]", "background: #1c211a;");
		addRule("#WorkflowScopeCheckBox", "color: " + textPrimary + "; font-size: 9.25pt; font-weight: 700;");
		addRule("#WorkflowScopeDescription", "color: " + textSecondary + "; font-size: 8.5pt;");
		addRule("#WorkflowScopeMetadata", "color: " + textMuted + "; font-size: 8.25pt; font-weight: 650; padding-left: 18px;");
		addRule(
		    "#WorkflowAutomationNote",
		    "background: #171a17; border: 1px solid " + divider + "; border-left: 2px solid " + accent
		        + "; border-radius: 4px; margin-top: 8px;");
		addRule("#WorkflowAutomationTitle", "color: " + accent + "; font-size: 8pt; font-weight: 700; letter-spacing: 0.25px;");
		addRule("#WorkflowAutomationDetail", "color: " + textSecondary + "; font-size: 8.5pt;");
		addRule("#ActionMetaPanel", "background: #181b19; border: none; border-top: 1px solid " + divider + ";");
		addRule("#ActionMetaTitle", "color: " + textSecondary + "; font-size: 7.75pt; font-weight: 700;");
		addRule("#ActionMetaText", "color: " + textBody + "; font-size: 7.75pt;");
		addRule("#ActionMetaDetail", "color: " + textMuted + "; font-size: 7.5pt;");
		addRule(
		    "#StatusRow",
		    "background: transparent; border: none; border-top: 1px solid " + divider + "; padding: 10px 12px; margin-top: 0;");
		addRule("#StatusLabel", "color: " + textPrimary + "; font-size: 9.25pt; font-weight: 650;");
		addRule("#InlineStatusValue", "background: transparent; border: none; padding: 0; font-size: 8.5pt; font-weight: 700;");
		addRule("#InlineStatusValue[State=\"ok\"]", "color: #9bcf68;");
		addRule("#InlineStatusValue[State=\"warning\"]", "color: #e3b95f;");
		addRule("#InlineStatusValue[State=\"running\"]", "color: #d8c996;");
		addRule("#InlineStatusValue[State=\"neutral\"]", "color: " + textSecondary + ";");
		addRule("#InlineStatusValue[State=\"bad\"]", "color: #ef8f86;");
		addRule("#StatusActionCell", "background: transparent; border: none;");
		addRule("#StatusDetail", "color: " + textMuted + "; font-size: 8.5pt;");
		addRule("#MapCatalogCard", "background: #1d201e; border: 1px solid #363b37; border-radius: 5px;");
		addRule("#MapCatalogCard:hover", "border-color: #596159;");
		addRule("#MapCatalogCard[State=\"ok\"]", "border-color: #3d4639;");
		addRule("#MapCatalogCard[State=\"warning\"]", "border-color: #504936;");
		addRule("#MapCardThumbnail", "background: #090b0a; border: none; border-right: 1px solid #343934;");
		addRule("#MapCardBody", "background: #1d201e; border: none;");
		addRule("#MapCatalogCard:hover #MapCardBody", "background: #222624;");
		addRule(
		    "#MapCardTitle",
		    "background: transparent; color: #f4f7f1; border: none; border-left: 2px solid #596159; font-size: 10.5pt; font-weight: "
		    "700; padding: 1px 0 1px 9px;");
		addRule("#MapCardTitle[State=\"ok\"]", "border-left-color: " + accent + ";");
		addRule("#MapCardTitle[State=\"warning\"]", "border-left-color: " + warning + ";");
		addRule("#MapCardDescription", "color: " + textBody + "; font-size: 8.75pt; line-height: 132%; padding: 1px 1px;");
		addRule("#MapCardMeta", "color: " + textMuted + "; font-size: 8pt; padding: 0 1px;");
		addRule(
		    "#MapCardSourceButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: 1px solid #4b524d; border-radius: 4px; padding: 2px 8px; font-size: 8.25pt; font-weight: 650;");
		addRule("#MapCardSourceButton:hover", "background: #32373a; color: #ffffff; border-color: #707a75;");
		addRule(
		    "#MapCardActionButton",
		    "background: #26321f; color: #f4f7f1; border: 1px solid " + accent
		        + "; border-radius: 4px; padding: 2px 8px; font-size: 8.25pt; font-weight: 700;");
		addRule("#MapCardActionButton:hover", "background: " + accent + "; color: #071006; border-color: #a8ed4f;");
		addRule("#MapCardActionButton:disabled", "background: #2d312d; color: " + textMuted + "; border-color: #41483e;");
		addRule("#StatusActionButton", "padding: 2px 8px; font-size: 8.25pt; font-weight: 700;");
		addRule("#StatusActionButton[ActionState=\"warning\"]", "background: " + accent + "; color: #071006; border: 1px solid #92d83a;");
		addRule("#StatusActionButton[ActionState=\"warning\"]:hover", "background: " + accentHover + "; border-color: #a8ed4f;");
		addRule(
		    "#StatusActionButton[ActionState=\"ok\"]",
		    "background: transparent; color: " + textSecondary + "; border: 1px solid #515950;");
		addRule("#StatusActionButton[ActionState=\"ok\"]:hover", "background: #30362e; color: " + textBody + "; border-color: #71806c;");
		addRule("#StatusActionButton[ActionState=\"running\"]", "background: #3c351f; color: #d8c996; border: 1px solid #736438;");
		addRule("#StatusActionButton:disabled", "background: #2d312d; color: " + textMuted + "; border-color: #41483e;");
		addRule("#ActionRow", "background: transparent; border: none; padding: 4px 0;");
		addRule("#ActionTitle", "color: " + textPrimary + "; font-size: 8.5pt; font-weight: 700;");
		addRule(
		    "#InlineActionButton",
		    "background: #2b2f2a; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; border-top-color: #42493f; padding: 4px 10px; min-width: 116px;");
		addRule("#InlineActionButton:hover", "background: " + panelHover + ";");
		addRule("#MutedLabel", "color: " + textMuted + "; padding: 4px 0;");
		addRule(
		    "#ActivitySummary",
		    "color: " + textSecondary + "; background: transparent; font-size: 8.25pt; font-weight: 600; padding: 0 0 2px 0;");
		addRule(
		    "#WorkflowGroupButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: none; border-left: 3px solid transparent; border-radius: 4px; margin: 2px 8px; padding: 0 14px; "
		          "text-align: left; font-size: 9.25pt; font-weight: 650;");
		addRule("#WorkflowGroupButton:hover", "background: #1d211e; color: " + textPrimary + "; border-left: 3px solid #465044;");
		addRule(
		    "#WorkflowGroupButton[ActiveState=\"true\"]",
		    "background: #20251f; color: " + textPrimary + "; border-left: 3px solid " + accent + ";");
		addRule(
		    "#WorkflowGroupButton:focus",
		    "background: #1d211e; color: " + textPrimary + "; border: none; border-left: 3px solid #65715f;");
		addRule(
		    "#WorkflowGroupButton[ActiveState=\"true\"]:focus",
		    "background: #20251f; color: " + textPrimary + "; border: none; border-left: 3px solid " + accent + ";");
		addRule(
		    "#WorkflowButton",
		    "background: transparent; color: " + textSecondary
		        + "; border: none; border-bottom: 3px solid transparent; padding: 11px 17px 9px 17px; text-align: center; font-size: "
		          "9.25pt; font-weight: 800;");
		addRule("#WorkflowButton:hover", "background: #1b1e1b; color: " + textPrimary + ";");
		addRule("#WorkflowButton:checked", "background: transparent; border-bottom: 3px solid " + accent + "; color: #ffffff;");
		addRule(
		    "#WorkflowButton:focus",
		    "background: #1b1e1b; color: " + textPrimary + "; border: none; border-bottom: 3px solid #56614c;");
		addRule(
		    "#WorkflowButton:checked:focus",
		    "background: transparent; color: #ffffff; border: none; border-bottom: 3px solid " + accent + ";");
		addRule(
		    "QPushButton",
		    "background: " + primary
		        + "; color: #071006; border: 1px solid #92d83a; border-radius: 4px; padding: 6px 15px; font-weight: 700;");
		addRule("QPushButton:hover", "background: " + primaryHover + ";");
		addRule("QPushButton:focus", "border: 1px solid " + focus + ";");
		addRule(
		    "QPushButton:disabled",
		    "background: #2d312d; border: 1px solid " + border + "; border-top-color: #41483e; color: " + textMuted + ";");
		addRule("#PrimaryActionButton", "background: " + primary + "; color: #071006; padding: 0; font-weight: 700;");
		addRule("#PrimaryActionButton:hover", "background: " + primaryHover + ";");
		addRule(
		    "#SecondaryButton",
		    "background: #2a2d2a; color: " + textBody + "; border: 1px solid " + borderSoft
		        + "; border-radius: 4px; padding: 0 10px; font-size: 8.5pt; font-weight: 650;");
		addRule("#PrimaryActionButton[ActionTone=\"destructive\"]", "background: #b64f48; color: #ffffff; border: 1px solid #dc7067;");
		addRule("#PrimaryActionButton[ActionTone=\"destructive\"]:hover", "background: #ce5e56; border-color: #ef837b;");
		addRule("#SecondaryButton[ActionTone=\"destructive\"]", "background: transparent; color: #e6a19b; border: 1px solid #70413d;");
		addRule("#SecondaryButton[ActionTone=\"destructive\"]:hover", "background: #30201f; border-color: #9a5650;");
		addRule(
		    "QComboBox, QLineEdit, QTextEdit",
		    "background: " + field + "; border: 1px solid " + borderStrong + "; border-radius: 4px; padding: 5px 9px; color: " + textBody
		        + "; selection-background-color: " + selection + "; min-height: 26px;");
		addRule("QComboBox:focus, QLineEdit:focus, QTextEdit:focus", "border: 1px solid " + focus + ";");
		addRule("QComboBox:disabled", "background: " + shell + "; border: 1px solid " + border + "; color: " + textMuted + ";");
		addRule("QCheckBox", "spacing: 8px; padding: 0; color: " + textBody + "; font-size: 9pt;");
		addRule("QCheckBox::indicator", "width: 14px; height: 14px; border-radius: 3px;");
		addRule("QCheckBox::indicator:unchecked", "background: #151715; border: 1px solid #596056;");
		addRule("QCheckBox::indicator:unchecked:hover", "border: 1px solid " + accent + ";");
		addRule("QCheckBox::indicator:checked", "background: " + accent + "; border: 1px solid #9bdd42;");
		addRule("QCheckBox::indicator:disabled", "background: #242724; border: 1px solid #3a3f39;");
		addRule("QCheckBox:focus", "border: 1px solid " + focus + "; border-radius: 2px; color: " + textPrimary + ";");
		addRule("QCheckBox:disabled", "color: " + textMuted + ";");
		addRule("#WarningCheckBox", "color: " + warning + ";");
		addRule("#DestructiveCheckBox", "color: " + destructive + ";");
		addRule("QListWidget", "background: transparent; border: none; border-radius: 0; padding: 0; outline: 0;");
		addRule("QListWidget::item", "padding: 3px 4px; border-radius: 0; color: " + textBody + ";");
		addRule("QListWidget::item:selected", "background: " + selection + "; color: #ffffff;");
		addRule("QScrollBar:vertical", "background: transparent; width: 10px; margin: 2px 1px;");
		addRule("QScrollBar::handle:vertical", "background: #434b42; border-radius: 4px; min-height: 44px;");
		addRule("QScrollBar::handle:vertical:hover", "background: #626d5f;");
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
		addRule("#ActivityRunTitle", "color: " + textBody + "; font-size: 8.25pt; font-weight: 650; padding: 0; margin: 0;");
		addRule("#ActivityRunState", "color: " + textMuted + "; font-size: 7.75pt; font-weight: 700; padding: 0; margin: 0;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunTitle", "color: #ffffff;");
		addRule("#ActivityRunRow[Selected=\"true\"] #ActivityRunState", "color: #dff3cf;");
		addRule(
		    "#OperationOutput",
		    "background: transparent; border: none; border-radius: 0; padding: 2px 0 0 0; font-family: 'Cascadia Mono'; font-size: "
		    "8.5pt;");

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
