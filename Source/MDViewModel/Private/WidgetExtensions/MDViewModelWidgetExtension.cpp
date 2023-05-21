#include "WidgetExtensions/MDViewModelWidgetExtension.h"

#include "MDViewModelModule.h"
#include "Launch/Resources/Version.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

void UMDViewModelWidgetExtension::Initialize()
{
	Super::Initialize();

	PopulateViewModels();
}

void UMDViewModelWidgetExtension::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		CleanUpViewModels();

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

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModel(UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModel))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			UMDViewModelBase* OldViewModel = ViewModels.FindRef(Key);
			ViewModels.FindOrAdd(Key) = ViewModel;

			if (ViewModel != OldViewModel)
			{
				BroadcastViewModelChanged(OldViewModel, ViewModel, ViewModelClass, ViewModelName);
			}

			return ViewModel;
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModelOfClass(UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), ViewModelClass);
		if (IsValid(ViewModel))
		{
			ViewModel->InitializeViewModelWithContext(ViewModelSettings, ContextObject);
			return SetViewModel(ViewModel, ViewModelClass, ViewModelName);
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName) const
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			return ViewModels.FindRef(Key);
		}
	}

	return nullptr;
}

void UMDViewModelWidgetExtension::ClearViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (ViewModelClass != nullptr)
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			UMDViewModelBase* OldViewModel = ViewModels.FindRef(Key);
			ViewModels.FindOrAdd(Key) = nullptr;

			BroadcastViewModelChanged(OldViewModel, nullptr, ViewModelClass, ViewModelName);
		}
	}
}

void UMDViewModelWidgetExtension::OnProviderViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass, FGameplayTag ProviderTag)
{
	UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

	const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	ViewModelModule.SearchViewModelAssignments(Assignments, GetUserWidget()->GetClass(), ViewModelClass, ProviderTag);

	UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(ProviderTag);
	if (!IsValid(Provider))
	{
		return;
	}

	for (const auto& Pair : Assignments)
	{
		Provider->SetViewModel(*Widget, Pair.Key, Pair.Value);
	}
}

FDelegateHandle UMDViewModelWidgetExtension::ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		return OnViewModelSetDelegates.FindOrAdd(Key).Add(MoveTemp(Delegate));
	}

	return {};
}

void UMDViewModelWidgetExtension::StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Key))
		{
			if (Delegate->Remove(Handle))
			{
				Handle.Reset();
			}
		}
	}
}

void UMDViewModelWidgetExtension::StopListeningForAllNativeViewModelsChanged(const void* BoundObject)
{
	for (auto& Pair : OnViewModelSetDelegates)
	{
		Pair.Value.RemoveAll(BoundObject);
	}
}

void UMDViewModelWidgetExtension::ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		OnViewModelSetDynamicDelegates.FindOrAdd(Key).Add(MoveTemp(Delegate));
	}
}

void UMDViewModelWidgetExtension::StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Key))
		{
			Delegates->Remove(Delegate);
		}
	}
}

void UMDViewModelWidgetExtension::StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject)
{
	for (auto& Pair : OnViewModelSetDynamicDelegates)
	{
		for (auto It = Pair.Value.CreateIterator(); It; ++It)
		{
			if (It->IsBoundToObject(BoundObject))
			{
				It.RemoveCurrent();
			}
		}
	}
}

void UMDViewModelWidgetExtension::BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	UUserWidget* OwnerWidget = GetUserWidget();
	if (IsValid(OldViewModel))
	{
		OldViewModel->OnUnsetFromWidget(OwnerWidget);
	}

	if (IsValid(NewViewModel))
	{
		NewViewModel->OnSetOnWidget(OwnerWidget);
	}

	const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
	if (const FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Key))
	{
		Delegate->Broadcast(OldViewModel, NewViewModel);
	}

	if (const TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Key))
	{
		for (const FMDVMOnViewModelSetDynamic& Delegate : *Delegates)
		{
			Delegate.ExecuteIfBound(OldViewModel, NewViewModel);
		}
	}
}

void UMDViewModelWidgetExtension::PopulateViewModels()
{
	UUserWidget* Widget = GetUserWidget();
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

	const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	ViewModelModule.GetViewModelAssignmentsForWidgetClass(Widget->GetClass(), Assignments);

	for (const auto& Pair : Assignments)
	{
		UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Pair.Key.ProviderTag);
		if (ensureMsgf(IsValid(Provider), TEXT("A View Model Provider with tag [%s] was not found"), *Pair.Key.ProviderTag.ToString()))
		{
			Provider->SetViewModel(*Widget, Pair.Key, Pair.Value);
			Provider->OnViewModelUpdated.AddUObject(this, &UMDViewModelWidgetExtension::OnProviderViewModelUpdated, Pair.Key.ProviderTag);
		}
	}
}

void UMDViewModelWidgetExtension::CleanUpViewModels()
{
	// Broadcast out that we're null-ing out viewmodels
	{
		for (auto It = OnViewModelSetDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = ViewModels.FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				It.Value().Broadcast(ViewModel, nullptr);
			}
		}
	}

	// Viewmodels themselves
	{
		UUserWidget* OwnerWidget = GetUserWidget();
		for (auto It = ViewModels.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = It.Value();
			if (IsValid(ViewModel))
			{
				ViewModel->OnUnsetFromWidget(OwnerWidget);
			}
		}

		ViewModels.Reset();
	}
}
