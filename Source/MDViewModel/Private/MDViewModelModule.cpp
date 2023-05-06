#include "MDViewModelModule.h"

#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Util/MDViewModelAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

#define LOCTEXT_NAMESPACE "FMDViewModelModule"

void FMDViewModelModule::StartupModule()
{
	FWorldDelegates::OnStartGameInstance.AddRaw(this, &FMDViewModelModule::OnGameStarted);
}

void FMDViewModelModule::ShutdownModule()
{
	ViewModelProviders.Reset();
	NativelyAssignedViewModels.Reset();

	FWorldDelegates::OnStartGameInstance.RemoveAll(this);
}

void FMDViewModelModule::RegisterViewModelProvider(const FGameplayTag& ProviderTag, TSharedRef<FMDViewModelProviderBase> Provider)
{
	ensureMsgf(!ViewModelProviders.Contains(ProviderTag), TEXT("A Provider with Tag [%s] has already been registered and will be overridden"), *ProviderTag.ToString());

	if (ensure(ProviderTag.IsValid()))
	{
		ViewModelProviders.Add(ProviderTag, Provider);
	}
}

void FMDViewModelModule::UnregisterViewModelProvider(const FGameplayTag& ProviderTag)
{
	ViewModelProviders.Remove(ProviderTag);
}

void FMDViewModelModule::RegisterNativeAssignment(TSubclassOf<UUserWidget> WidgetClass, FMDViewModelAssignment&& Assignment, FMDViewModelAssignmentData&& Data)
{
	if (ensure(IsValid(WidgetClass)) && ensure(WidgetClass->HasAllClassFlags(CLASS_Native)) && ensure(Assignment.IsValid()) && ensure(Assignment.ViewModelClass->HasAllClassFlags(CLASS_Native)))
	{
		NativelyAssignedViewModels.FindOrAdd(WidgetClass).Add(MoveTemp(Assignment), MoveTemp(Data));
	}
}

void FMDViewModelModule::UnregisterNativeAssignment(TSubclassOf<UUserWidget> WidgetClass)
{
	NativelyAssignedViewModels.Remove(WidgetClass);
}

void FMDViewModelModule::GetNativeAssignments(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutAssignments) const
{
	RequestNativeAssignments();

	if (WidgetClass != nullptr)
	{
		for (UClass* CheckClass = WidgetClass.Get(); CheckClass != UWidget::StaticClass(); CheckClass = CheckClass->GetSuperClass())
		{
			if (const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>* Assignments = NativelyAssignedViewModels.Find(CheckClass))
			{
				OutAssignments.Append(*Assignments);
			}
		}
	}
}

void FMDViewModelModule::GetViewModelClassesForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const
{
	if (WidgetClass != nullptr)
	{
		for (UClass* CheckClass = WidgetClass.Get(); CheckClass != UWidget::StaticClass(); CheckClass = CheckClass->GetSuperClass())
		{
			if (const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>* Assignments = NativelyAssignedViewModels.Find(CheckClass))
			{
				for (const auto& Pair : *Assignments)
				{
					OutViewModelClasses.Add(Pair.Key.ViewModelClass);
				}
			}
		}

		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
		{
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
			{
				ClassExtension->GetViewModelClasses(OutViewModelClasses);
			}
		}
	}
}

void FMDViewModelModule::GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	if (WidgetClass != nullptr)
	{
		for (UClass* CheckClass = WidgetClass.Get(); CheckClass != UWidget::StaticClass(); CheckClass = CheckClass->GetSuperClass())
		{
			if (const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>* Assignments = NativelyAssignedViewModels.Find(CheckClass))
			{
				OutViewModelAssignments.Append(*Assignments);
			}
		}

		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
		{
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
			{
				// TODO - Deal with Native <-> BP collisions
				OutViewModelAssignments.Append(ClassExtension->GetAssignments());
			}
		}
	}
}

void FMDViewModelModule::SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	auto DoesAssignmentMatchSearch = [&](const FMDViewModelAssignment& Assignment)
	{
		if (ProviderTag.IsValid() && !Assignment.ProviderTag.MatchesTagExact(ProviderTag))
		{
			return false;
		}

		if (ViewModelName != NAME_None && ViewModelName != Assignment.ViewModelName)
		{
			return false;
		}

		if (ViewModelClass != nullptr && ViewModelClass != Assignment.ViewModelClass)
		{
			return false;
		}

		return true;
	};

	for (auto Pair : NativelyAssignedViewModels)
	{
		if (WidgetClass != nullptr && WidgetClass != Pair.Key)
		{
			continue;
		}

		for (const auto& AssignmentPair : Pair.Value)
		{
			if (DoesAssignmentMatchSearch(AssignmentPair.Key))
			{
				OutViewModelAssignments.Add(AssignmentPair.Key, AssignmentPair.Value);
			}
		}
	}

	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
		{
			ClassExtension->SearchAssignments(OutViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
		}
	}
}

bool FMDViewModelModule::DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass) const
{
	UClass* Class = WidgetClass;
	while (Class != nullptr)
	{
		if (const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>* Assignments = NativelyAssignedViewModels.Find(Class))
		{
			if (!Assignments->IsEmpty())
			{
				return true;
			}
		}

		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
		{
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
			{
				if (ClassExtension->HasAssignments())
				{
					return true;
				}
			}
		}

		Class = Class->GetSuperClass();
	}

	return false;
}

TSharedPtr<FMDViewModelProviderBase> FMDViewModelModule::GetViewModelProvider(const FGameplayTag& ProviderTag) const
{
	return ViewModelProviders.FindRef(ProviderTag);
}

void FMDViewModelModule::OnGameStarted(UGameInstance* GameInstance)
{
	for (auto Pair : ViewModelProviders)
	{
		if (const TSharedPtr<FMDViewModelProviderBase>& Provider = Pair.Value)
		{
			Provider->InitializeProvider(GameInstance);
		}
	}
}

void FMDViewModelModule::RequestNativeAssignments() const
{
	if (!bHasRequestedNativeAssignments)
	{
		OnNativeAssignmentsRequested.Broadcast();
		bHasRequestedNativeAssignments = true;
		OnNativeAssignmentsRequested.Clear();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDViewModelModule, MDViewModel)