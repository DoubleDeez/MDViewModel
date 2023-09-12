#include "ViewModelTab/MDViewModelList.h"

#include "BlueprintEditor.h"
#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "ScopedTransaction.h"
#include "Subsystems/MDViewModelGraphSubsystem.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelAssignmentDialog.h"
#include "ViewModelTab/MDViewModelListItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

SMDViewModelList::~SMDViewModelList()
{
	if (IsValid(GEngine))
	{
		if (UMDViewModelGraphSubsystem* MDVMGraphSubsystem = GEngine->GetEngineSubsystem<UMDViewModelGraphSubsystem>())
		{
			MDVMGraphSubsystem->OnAssignmentsChanged.RemoveAll(this);
		}
	}
}

void SMDViewModelList::Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& InBlueprintEditor)
{
	check(InBlueprintEditor.IsValid());
	OnViewModelSelected = InArgs._OnViewModelSelected;
	BlueprintEditorPtr = InBlueprintEditor;

	if (IsValid(GEngine))
	{
		if (UMDViewModelGraphSubsystem* MDVMGraphSubsystem = GEngine->GetEngineSubsystem<UMDViewModelGraphSubsystem>())
		{
			MDVMGraphSubsystem->OnAssignmentsChanged.AddSP(this, &SMDViewModelList::OnAssignmentsChanged);
		}
	}

	PopulateAssignments();

	ChildSlot
	.Padding(4.f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(4.f)
			.BorderImage(&FAppStyle::Get().GetWidgetStyle<FTableViewStyle>("ListView").BackgroundBrush)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Assigned View Models"))
				.ToolTipText(INVTEXT("These view models will be set on the widget at runtime based on the selected providers."))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(0, 4.f)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(AssignmentList, SListView<TSharedPtr<FMDViewModelEditorAssignment>>)
			.ListItemsSource(&Assignments)
			.OnGenerateRow(this, &SMDViewModelList::OnGenerateRow)
			.OnSelectionChanged(this, &SMDViewModelList::OnItemSelected)
			.OnContextMenuOpening(this, &SMDViewModelList::OnContextMenuOpening)
			.AllowOverscroll(EAllowOverscroll::No)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.OnClicked(this, &SMDViewModelList::OnAddViewModel)
			.IsEnabled(this, &SMDViewModelList::CanAddViewModel)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(0, 0, 4.f, 0)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush(TEXT("EditableComboBox.Add")))
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock).Text(INVTEXT("Add View Model"))
				]
			]
		]
	];
}

void SMDViewModelList::RefreshList()
{
	PopulateAssignments();
	if (AssignmentList.IsValid())
	{
		AssignmentList->RequestListRefresh();
	}
}

void SMDViewModelList::OnItemSelected(TSharedPtr<FMDViewModelEditorAssignment> Item, ESelectInfo::Type SelectInfo)
{
	OnViewModelSelected.ExecuteIfBound(Item.Get());
}

TSharedRef<ITableRow> SMDViewModelList::OnGenerateRow(TSharedPtr<FMDViewModelEditorAssignment> Item, const TSharedRef<STableViewBase>& OwningTable)
{
	return SNew(SMDViewModelListItem, OwningTable, Item, BlueprintEditorPtr)
		.OnDuplicateItemRequested(this, &SMDViewModelList::OnDuplicateItem, Item)
		.OnEditItemRequested(this, &SMDViewModelList::OnEditItem, Item)
		.OnDeleteItemConfirmed(this, &SMDViewModelList::OnDeleteItem, Item);
}

TSharedPtr<SWidget> SMDViewModelList::OnContextMenuOpening()
{
	FMenuBuilder ContextMenuBuilder(true, nullptr);

	TArray<TSharedPtr<FMDViewModelEditorAssignment>> SelectedItems = AssignmentList->GetSelectedItems();
	if (SelectedItems.Num() == 1 && SelectedItems[0].IsValid())
	{
		const TSharedPtr<SMDViewModelListItem> ListItem = StaticCastSharedPtr<SMDViewModelListItem>(AssignmentList->WidgetFromItem(SelectedItems[0]));
		if (ListItem.IsValid())
		{
			ListItem->OnContextMenuOpening(ContextMenuBuilder);
		}
	}
	
	ContextMenuBuilder.BeginSection("ViewModelList", INVTEXT("View Model List"));
	{
		ContextMenuBuilder.AddMenuEntry(
			INVTEXT("Paste Assignment"),
			INVTEXT("Paste a view model assignment from the clipboard."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GenericCommands.Paste")),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMDViewModelList::OnPasteClicked),
				FCanExecuteAction::CreateSP(this, &SMDViewModelList::CanPaste)
			)
		);
	}
	ContextMenuBuilder.EndSection();

	return ContextMenuBuilder.MakeWidget();
}

void SMDViewModelList::PopulateAssignments()
{
	UClass* GeneratedClass = GetGeneratedClass();
	if (!ensure(IsValid(GeneratedClass)))
	{
		return;
	}
	
	IMDViewModelAssignableInterface* Extension = FMDViewModelGraphStatics::GetAssignableInterface(GetBlueprint());
	
	TArray<UBlueprintGeneratedClass*> Hierarchy;
	UBlueprint::GetBlueprintHierarchyFromClass(GeneratedClass, Hierarchy);

	TArray<UClass*> ChildFirstSortedAncestry;
	TMap<UClass*, TArray<FMDViewModelEditorAssignment>> AllAssignments;

	ChildFirstSortedAncestry.Add(GeneratedClass);
	AllAssignments.Add(GeneratedClass, Extension ? Extension->GetAssignments() : TArray<FMDViewModelEditorAssignment>{});

	// Skip the first one since that's just ThisBP
	for (int32 i = 1; i < Hierarchy.Num(); ++i)
	{
		if (UBlueprintGeneratedClass* SuperClass = Hierarchy[i])
		{
			ChildFirstSortedAncestry.Add(SuperClass);
			
			TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> SuperAssignments;
			MDViewModelUtils::GetViewModelAssignments(SuperClass, SuperAssignments);
		
			TArray<FMDViewModelEditorAssignment>& SuperEditorAssignments = AllAssignments.Add(SuperClass);
			for (const auto& Pair : SuperAssignments)
			{
				FMDViewModelEditorAssignment NewAssignment;
				NewAssignment.Assignment = Pair.Key;
				NewAssignment.Data = Pair.Value;
				NewAssignment.SuperAssignmentOwner = SuperClass;
				SuperEditorAssignments.Emplace(MoveTemp(NewAssignment));
			}
		}
	}

	TArray<UClass*> EldestFirstSortedAncestry = ChildFirstSortedAncestry;
	Algo::Reverse(EldestFirstSortedAncestry);

	// Populate assignment list started from eldest class (will be reversed below for displaying to the user)
	TArray<FMDViewModelEditorAssignment> ActiveAssignments;
	TArray<FMDViewModelEditorAssignment> OverridenAssignments;
	for (UClass* AssignmentOwner : EldestFirstSortedAncestry)
	{
		if (const TArray<FMDViewModelEditorAssignment>* AssignmentsPtr = AllAssignments.Find(AssignmentOwner))
		{
			for (const FMDViewModelEditorAssignment& EditorAssignment : *AssignmentsPtr)
			{
				if (const FMDViewModelEditorAssignment* ExistingAssignment = ActiveAssignments.FindByKey(EditorAssignment))
				{
					if (AssignmentOwner == GeneratedClass)
					{
						OverridenAssignments.Add(EditorAssignment);
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
						UE_LOGFMT(LogMDViewModel, Warning, "Blueprint {BPName} has a view model assignment {Assignment} that is being overridden by a matching assignment from its ancestor {SuperName}",
							("BPName", GetPathNameSafe(GetBlueprint())),
							("Assignment", EditorAssignment.Assignment),
							("SuperName", GetPathNameSafe(ExistingAssignment->SuperAssignmentOwner))
						);
#else
						UE_LOG(LogMDViewModel, Warning, TEXT("Blueprint [%s] has a view model assignment [%s (%s)] that is being overridden by a matching assignment from its ancestor [%s]"),
							*GetPathNameSafe(GetBlueprint()),
							*GetNameSafe(EditorAssignment.Assignment.ViewModelClass),
							*EditorAssignment.Assignment.ViewModelName.ToString(),
							*GetPathNameSafe(ExistingAssignment->SuperAssignmentOwner)
						);
#endif
							
					}
				}
				else
				{
					ActiveAssignments.Add(EditorAssignment);
				}
			}
		}
	}

	// Display assignments from this blueprint first, but maintain the order they were added in
	ActiveAssignments.Sort([&](const FMDViewModelEditorAssignment& A, const FMDViewModelEditorAssignment& B)
	{
		// Non-Super first
		const bool bAIsSuper = IsValid(A.SuperAssignmentOwner); 
		const bool bBIsSuper = IsValid(B.SuperAssignmentOwner); 
		if (bAIsSuper != bBIsSuper)
		{
			return bBIsSuper;
		}

		// Child-most class first
		if (bAIsSuper && bBIsSuper && (A.SuperAssignmentOwner != B.SuperAssignmentOwner))
		{
			return A.SuperAssignmentOwner->IsChildOf(B.SuperAssignmentOwner);
		}

		// Maintain order they were added in
		const UClass* AssignmentOwner = bAIsSuper ? A.SuperAssignmentOwner.Get() : GeneratedClass;
		const TArray<FMDViewModelEditorAssignment>* AssignmentsPtr = AllAssignments.Find(AssignmentOwner);
		if (AssignmentsPtr == nullptr)
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			UE_LOGFMT(LogMDViewModel, Error, "Failed to sort assignments for ViewModelList. Class [{ClassName}] was not in AllAssignments map",
				("ClassName", GetNameSafe(AssignmentOwner))
			);
#else
			UE_LOG(LogMDViewModel, Error, TEXT("Failed to sort assignments for ViewModelList. Class [%s] was not in AllAssignments map"),
				*GetNameSafe(AssignmentOwner)
			);
#endif
			return true;
		}

		const int32 AIndex = AssignmentsPtr->IndexOfByKey(A);
		if (AIndex < 0)
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			UE_LOGFMT(LogMDViewModel, Error, "Failed to sort assignments for ViewModelList. Assignment [{Assignment}] was not in Class's [{ClassName}] assignments list",
				("Assignment", A.Assignment),
				("ClassName", GetNameSafe(AssignmentOwner))
			);
#else
			UE_LOG(LogMDViewModel, Error, TEXT("Failed to sort assignments for ViewModelList. Assignment [%s (%s)] was not in Class's [%s] assignments list"),
				*GetNameSafe(A.Assignment.ViewModelClass),
				*A.Assignment.ViewModelName.ToString(),
				*GetNameSafe(AssignmentOwner)
			);
#endif
			return false;
		}
		
		const int32 BIndex = AssignmentsPtr->IndexOfByKey(B);
		if (BIndex < 0)
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			UE_LOGFMT(LogMDViewModel, Error, "Failed to sort assignments for ViewModelList. Assignment [{Assignment}] was not in Class's [{ClassName}] assignments list",
				("Assignment", B.Assignment),
				("ClassName", GetNameSafe(AssignmentOwner))
			);
#else
			UE_LOG(LogMDViewModel, Error, TEXT("Failed to sort assignments for ViewModelList. Assignment [%s (%s)] was not in Class's [%s] assignments list"),
				*GetNameSafe(B.Assignment.ViewModelClass),
				*B.Assignment.ViewModelName.ToString(),
				*GetNameSafe(AssignmentOwner)
			);
#endif
			return true;
		}

		return AIndex < BIndex;
	});

	// Keep existing sharedptrs active and reuse them when possible
	TArray<TSharedPtr<FMDViewModelEditorAssignment>> OldAssignment = Assignments;
	Assignments.Reset(ActiveAssignments.Num());

	// TODO - Display OverridenAssignments right after their colliding assignment so User can decide how they want to handle the collision

	for (const FMDViewModelEditorAssignment& MyAssignment : ActiveAssignments)
	{
		const TSharedPtr<FMDViewModelEditorAssignment>* AssignmentPtr = OldAssignment.FindByPredicate([&MyAssignment](const TSharedPtr<FMDViewModelEditorAssignment>& Assignment)
		{
			return Assignment->Assignment == MyAssignment.Assignment && Assignment->SuperAssignmentOwner == MyAssignment.SuperAssignmentOwner;
		});

		if (AssignmentPtr == nullptr)
		{
			Assignments.Add(MakeShared<FMDViewModelEditorAssignment>(MyAssignment));
		}
		else
		{
			*(AssignmentPtr->Get()) = MyAssignment;
			Assignments.Add(*AssignmentPtr);
		}
	}
}

void SMDViewModelList::OnAssignmentsChanged(UBlueprint* Blueprint)
{
	if (Blueprint == GetBlueprint())
	{
		RefreshList();
	}
}

FReply SMDViewModelList::OnAddViewModel()
{
	SMDViewModelAssignmentDialog::OpenAssignmentDialog(GetBlueprint());
	if (const TSharedPtr<SMDViewModelAssignmentDialog> ActiveDialog = SMDViewModelAssignmentDialog::GetActiveDialog())
	{
		if (!ActiveDialog->OnAssignmentAdded.IsBoundToObject(this))
		{
			ActiveDialog->OnAssignmentAdded.AddSP(this, &SMDViewModelList::OnAssignmentAdded);
		}
	}

	return FReply::Handled();
}

bool SMDViewModelList::CanAddViewModel() const
{
	return !GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == nullptr;
}

void SMDViewModelList::OnAssignmentAdded(const FMDViewModelEditorAssignment& Assignment)
{
	if (AssignmentList.IsValid())
	{
		const TSharedPtr<FMDViewModelEditorAssignment>* AssignmentPtr = Assignments.FindByPredicate(
			[&Assignment](const TSharedPtr<FMDViewModelEditorAssignment>& AssignmentPtr)
			{
				return AssignmentPtr.IsValid() && *AssignmentPtr == Assignment;
			}
		);
		
		if (AssignmentPtr != nullptr)
		{
			AssignmentList->SetSelection(*AssignmentPtr);
		}
	}
}

void SMDViewModelList::OnPasteClicked()
{
	FString AssignmentString;
	FPlatformApplicationMisc::ClipboardPaste(AssignmentString);
	
	FMDViewModelEditorAssignment Assignment;
	UScriptStruct* AssignmentStruct = FMDViewModelEditorAssignment::StaticStruct();
	AssignmentStruct->ImportText(*AssignmentString, &Assignment, nullptr, PPF_Copy, GError, AssignmentStruct->GetName());
	
	Assignment.SuperAssignmentOwner = nullptr;

	if (IMDViewModelAssignableInterface* Extension = FMDViewModelGraphStatics::GetOrCreateAssignableInterface(GetBlueprint()))
	{
		if (Extension->DoesContainViewModelAssignment(Assignment.Assignment.ViewModelClass, FGameplayTag::EmptyTag, Assignment.Assignment.ViewModelName))
		{
			Assignment.Assignment.ViewModelName = *FString::Printf(TEXT("%s_Copy"), *Assignment.Assignment.ViewModelName.ToString());

			while (Extension->DoesContainViewModelAssignment(Assignment.Assignment.ViewModelClass, FGameplayTag::EmptyTag, Assignment.Assignment.ViewModelName))
			{
				FString AssignmentName = Assignment.Assignment.ViewModelName.ToString();
				if (AssignmentName.EndsWith(TEXT("_Copy")))
				{
					Assignment.Assignment.ViewModelName = *FString::Printf(TEXT("%s_01"), *AssignmentName);
				}
				else
				{
					int32 Index = INDEX_NONE;
					AssignmentName.FindLastChar(TEXT('_'), Index);
					const int32 SuffixNum = FCString::Atoi(*AssignmentName.RightChop(Index + 1));
					Assignment.Assignment.ViewModelName = *FString::Printf(TEXT("%s_%02d"), *AssignmentName.Left(Index), SuffixNum + 1);
				}
			}
		}
		
		FScopedTransaction Transaction = FScopedTransaction(INVTEXT("Pasted View Model Assignment"));
		Extension->AddAssignment(MoveTemp(Assignment));
	}
}

bool SMDViewModelList::CanPaste() const
{
	if (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld != nullptr)
	{
		return false;
	}
	
	FString AssignmentString;
	FPlatformApplicationMisc::ClipboardPaste(AssignmentString);

	if (AssignmentString.IsEmpty() || !AssignmentString.StartsWith(TEXT("(")))
	{
		return false;
	}

	FMDViewModelEditorAssignment Assignment;
	UScriptStruct* AssignmentStruct = FMDViewModelEditorAssignment::StaticStruct();
	AssignmentStruct->ImportText(*AssignmentString, &Assignment, nullptr, PPF_Copy, GError, AssignmentStruct->GetName());

	return Assignment.Assignment.IsValid();
}

void SMDViewModelList::OnDuplicateItem(TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	SMDViewModelAssignmentDialog::OpenDuplicateDialog(GetBlueprint(), Item);
}

void SMDViewModelList::OnEditItem(TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	SMDViewModelAssignmentDialog::OpenEditDialog(GetBlueprint(), Item);
}

void SMDViewModelList::OnDeleteItem(TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	if (Item.IsValid())
	{
		if (IMDViewModelAssignableInterface* Extension = FMDViewModelGraphStatics::GetAssignableInterface(GetBlueprint()))
		{
			FScopedTransaction Transaction = FScopedTransaction(INVTEXT("Removed View Model Assignment"));
			Extension->RemoveAssignment(*Item.Get());
		}
	}
}

UBlueprint* SMDViewModelList::GetBlueprint() const
{
	if (const TSharedPtr<FBlueprintEditor> BlueprintEditor = BlueprintEditorPtr.Pin())
	{
		return BlueprintEditor->GetBlueprintObj();
	}

	return nullptr;
}

UClass* SMDViewModelList::GetGeneratedClass() const
{
	if (const UBlueprint* Blueprint = GetBlueprint())
	{
		return Blueprint->GeneratedClass;
	}

	return nullptr;
}
