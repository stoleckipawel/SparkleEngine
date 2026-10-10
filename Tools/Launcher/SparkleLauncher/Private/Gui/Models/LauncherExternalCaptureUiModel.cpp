#include "LauncherExternalCaptureUiModel.h"
#include "ExternalCaptureDiscovery.h"

namespace SparkleLauncher
{
	static LauncherSelectionOption BuildExternalCaptureOption(ExternalCaptureProvider provider, const ExternalCaptureAvailability& availability)
	{
		QString name = QString::fromUtf8(ExternalCaptureProviderDisplayName(provider).data());
		if (provider == ExternalCaptureProvider::NsightGraphics)
		{
			name += " (Experimental SDK)";
		}

		return {name, QString::fromUtf8(ExternalCaptureProviderToString(provider).data()), QString::fromStdString(availability.Detail), availability.Available(), availability.Supported};
	}

	QVector<LauncherSelectionOption> BuildExternalCaptureOptions(std::string_view api, std::string_view profile)
	{
		QVector<LauncherSelectionOption> options;
		for (auto provider : ExternalCaptureProviders)
		{
			const auto availability = InspectExternalCaptureProvider(provider, api, profile);
			options.push_back(BuildExternalCaptureOption(provider, availability));
		}

		return options;
	}
}
