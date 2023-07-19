#include "MDViewModelModule.h"

#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Blueprint.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Util/MDViewModelAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

#define LOCTEXT_NAMESPACE "FMDViewModelModule"

void FMDViewModelModule::StartupModule()
{
}

void FMDViewModelModule::ShutdownModule()
{
	NativelyAssignedViewModels.Reset();
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
	RequestNativeAssignments();

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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
			{
				ClassExtension->GetViewModelClasses(OutViewModelClasses);
			}
		}
	}
}

void FMDViewModelModule::GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	RequestNativeAssignments();

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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
			{
				// TODO - Deal with Native <-> BP collisions
				OutViewModelAssignments.Append(ClassExtension->GetAssignments());
			}
		}
	}
}

void FMDViewModelModule::SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	RequestNativeAssignments();

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
		if (WidgetClass != nullptr && !WidgetClass->IsChildOf(Pair.Key))
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			ClassExtension->SearchAssignments(OutViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
		}
	}
}

bool FMDViewModelModule::DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass) const
{
	RequestNativeAssignments();

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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
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