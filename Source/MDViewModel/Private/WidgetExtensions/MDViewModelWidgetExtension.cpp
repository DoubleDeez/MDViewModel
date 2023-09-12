#include "WidgetExtensions/MDViewModelWidgetExtension.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"

void UMDViewModelWidgetExtension::Construct()
{
	Super::Construct();
	
	const UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	// TODO - Support DebugMode with editable debug view models
	if (Widget->IsPreviewTime())
	{
		return;
	}
#endif

	PopulateViewModels();
}

void UMDViewModelWidgetExtension::Destruct()
{
	CleanUpViewModels();
	
	Super::Destruct();
}

void UMDViewModelWidgetExtension::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnBeginDestroy.Broadcast();
	}
	
	Super::BeginDestroy();
}

UMDViewModelWidgetExtension* UMDViewModelWidgetExtension::GetOrCreate(UUserWidget* Widget)
{
	if (IsValid(Widget))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			return Extension;
		}

		return Widget->AddExtension<UMDViewModelWidgetExtension>();
	}

	return nullptr;
}

UObject* UMDViewModelWidgetExtension::GetOwningObject() const
{
	return GetUserWidget();
}

UGameInstance* UMDViewModelWidgetExtension::ResolveGameInstance() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetGameInstance();
	}

	return nullptr;
}

UWorld* UMDViewModelWidgetExtension::ResolveWorld() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetWorld();
	}

	return nullptr;
}

ULocalPlayer* UMDViewModelWidgetExtension::ResolveOwningLocalPlayer() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetOwningLocalPlayer();
	}

	return nullptr;
}

APlayerController* UMDViewModelWidgetExtension::ResolveOwningPlayer() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetOwningPlayer();
	}

	return nullptr;
}

TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& UMDViewModelWidgetExtension::GetViewModels()
{
	return ViewModels;
}
