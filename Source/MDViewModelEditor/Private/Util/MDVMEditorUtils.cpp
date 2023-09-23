#include "Util/MDVMEditorUtils.h"

#include "Framework/Notifications/NotificationManager.h"
#include "ISourceControlModule.h"
#include "HAL/PlatformFileManager.h"
#include "SSettingsEditorCheckoutNotice.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Notifications/SNotificationList.h"

void MDVMEditorUtils::SaveViewModelConfig(UMDViewModelBase* ViewModelCDO)
{
	if (IsValid(ViewModelCDO) && ViewModelCDO->IsTemplate())
	{
		const bool bIsUsingSourceControl = ISourceControlModule::Get().IsEnabled();
		const FString ConfigFilePath = FPaths::ConvertRelativePathToFull(ViewModelCDO->GetDefaultConfigFilename());
		const bool bIsNewFile = !FPlatformFileManager::Get().GetPlatformFile().FileExists(*ConfigFilePath);
		if (!bIsNewFile && !SettingsHelpers::IsCheckedOut(ConfigFilePath))
		{
			if (!bIsUsingSourceControl)
			{
				SettingsHelpers::MakeWritable(ConfigFilePath);
			}
			else if (!SettingsHelpers::CheckOutOrAddFile(ConfigFilePath, true))
			{
				FNotificationInfo Info = FNotificationInfo(FText::Format(INVTEXT("Could not check out config file {0}"), FText::FromString(FPaths::GetCleanFilename(ConfigFilePath))));
				Info.ExpireDuration = 6.0f;
				FSlateNotificationManager::Get().AddNotification(Info);

				SettingsHelpers::MakeWritable(ConfigFilePath);
			}
		}

		ViewModelCDO->TryUpdateDefaultConfigFile();

		if (bIsNewFile && bIsUsingSourceControl)
		{
			SettingsHelpers::CheckOutOrAddFile(ConfigFilePath, true);
		}
	}
}
