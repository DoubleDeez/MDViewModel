#include "MDViewModelEditorConfig.h"

#include "ClassViewerModule.h"
#include "GameplayTagsManager.h"

UMDViewModelEditorConfig::UMDViewModelEditorConfig()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("View Model Editor");
}

void UMDViewModelEditorConfig::PostInitProperties()
{
	Super::PostInitProperties();

	UGameplayTagsManager::Get().CallOrRegister_OnDoneAddingNativeTagsDelegate(
		FSimpleMulticastDelegate::FDelegate::CreateWeakLambda(this, [this]() { ReloadConfig(); }));
}

EClassViewerNameTypeToDisplay UMDViewModelEditorConfig::GetNameTypeToDisplay() const
{
	switch (NameTypeToDisplay)
	{
	case EMDVMClassViewerNameTypeToDisplay::Dynamic:
		return EClassViewerNameTypeToDisplay::Dynamic;
	case EMDVMClassViewerNameTypeToDisplay::ClassName:
		return EClassViewerNameTypeToDisplay::ClassName;
	case EMDVMClassViewerNameTypeToDisplay::DisplayName:
		return EClassViewerNameTypeToDisplay::DisplayName;
	default:
		checkf(false, TEXT("Unhandled EMDVMClassViewerNameTypeToDisplay type"));
	}

	return EClassViewerNameTypeToDisplay::DisplayName;
}
