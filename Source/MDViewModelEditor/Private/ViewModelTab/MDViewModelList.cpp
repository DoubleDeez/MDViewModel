#include "ViewModelTab/MDViewModelList.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Launch/Resources/Version.h"
#include "MDViewModelModule.h"
#include "ScopedTransaction.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModelTab/MDViewModelAssignmentDialog.h"
#include "ViewModelTab/MDViewmodelAssignmentEditorObject.h"
#include "ViewModelTab/MDViewModelListItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

SMDViewModelList::~SMDViewModelList()
{
	if (UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
	{
		BPExtension->OnAssignmentsChanged.RemoveAll(this);
	}
}

void SMDViewModelList::Construct(const FArguments& InArgs, UWidgetBlueprint* InBlueprint)
{
	check(IsValid(InBlueprint));
	WidgetBP = InBlueprint;
	WidgetClass = WidgetBP->GeneratedClass;
	OnViewModelSelected = InArgs._OnViewModelSelected;

	if (UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
	{
		BPExtension->SetFlags(RF_Transactional);
		BPExtension->OnAssignmentsChanged.AddSP(this, &SMDViewModelList::OnAssignmentsChanged);
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
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.OnClicked(this, &SMDViewModelList::OnAddViewModel)
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
		// TODO - Attempt to maintain the selected item
		AssignmentList->RequestListRefresh();
	}
}

void SMDViewModelList::OnItemSelected(TSharedPtr<FMDViewModelEditorAssignment> Item, ESelectInfo::Type SelectInfo)
{
	OnViewModelSelected.ExecuteIfBound(Item.Get());
}

TSharedRef<ITableRow> SMDViewModelList::OnGenerateRow(TSharedPtr<FMDViewModelEditorAssignment> Item, const TSharedRef<STableViewBase>& OwningTable)
{
	return SNew(STableRow<TSharedPtr<FMDViewModelEditorAssignment>>, OwningTable)
	[
		SNew(SMDViewModelListItem, Item)
		.OnEditItemRequested(this, &SMDViewModelList::OnEditItem, Item)
		.OnDeleteItemConfirmed(this, &SMDViewModelList::OnDeleteItem, Item)
	];
}

void SMDViewModelList::PopulateAssignments()
{
	TArray<FMDViewModelEditorAssignment> MyAssignments;
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> SuperAssignments;
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
		if (const UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			MyAssignments = BPExtension->GetAssignments();
		}

		for (auto* SuperClass = Cast<UWidgetBlueprintGeneratedClass>(WBGC->GetSuperClass()); IsValid(SuperClass); SuperClass = Cast<UWidgetBlueprintGeneratedClass>(SuperClass->GetSuperClass()))
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (const UMDViewModelWidgetClassExtension* ClassExtension = SuperClass->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (const UMDViewModelWidgetClassExtension* ClassExtension = SuperClass->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
			{
				// TODO - how to handle collisions from super classes
				//		- only happens if the super class adds an assignment that a child class already has
				SuperAssignments.Append(ClassExtension->GetAssignments());
			}
		}
	}

	auto FindAssignment = [&MyAssignments](const FMDViewModelAssignment& Assignment) -> FMDViewModelEditorAssignment*
	{
		for (FMDViewModelEditorAssignment& EditorAssignment : MyAssignments)
		{
			if (EditorAssignment.Assignment == Assignment)
			{
				return &EditorAssignment;
			}
		}

		return nullptr;
	};

	for (const auto& Pair : SuperAssignments)
	{
		if (FMDViewModelEditorAssignment* ExistingAssignment = FindAssignment(Pair.Key))
		{
			ExistingAssignment->bIsSuper = true;
		}
		else
		{
			FMDViewModelEditorAssignment NewAssignment;
			NewAssignment.Assignment = Pair.Key;
			NewAssignment.Data = Pair.Value;
			NewAssignment.bIsSuper = true;

			MyAssignments.Emplace(MoveTemp(NewAssignment));
		}
	}

	// Keep existing sharedptrs active
	TArray<TSharedPtr<FMDViewModelEditorAssignment>> OldAssignment = Assignments;
	Assignments.Reset(MyAssignments.Num());

	for (const FMDViewModelEditorAssignment& MyAssignment : MyAssignments)
	{
		const TSharedPtr<FMDViewModelEditorAssignment>* AssignmentPtr = OldAssignment.FindByPredicate([&MyAssignment](const TSharedPtr<FMDViewModelEditorAssignment>& Assignment)
		{
			return Assignment->Assignment == MyAssignment.Assignment;
		});

		if (AssignmentPtr == nullptr)
		{
			Assignments.Add(MakeShareable(new FMDViewModelEditorAssignment(MyAssignment)));
		}
		else
		{
			*(AssignmentPtr->Get()) = MyAssignment;
			Assignments.Add(*AssignmentPtr);
		}
	}
}

void SMDViewModelList::OnAssignmentsChanged()
{
	RefreshList();
}

FReply SMDViewModelList::OnAddViewModel()
{
	if (UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
	{
		SMDViewModelAssignmentDialog::OpenAssignmentDialog(BPExtension);
	}

	return FReply::Handled();
}

void SMDViewModelList::OnEditItem(TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	if (UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
	{
		SMDViewModelAssignmentDialog::OpenEditDialog(BPExtension, Item);
	}
}

void SMDViewModelList::OnDeleteItem(TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	if (Item.IsValid())
	{
		if (UMDViewModelWidgetBlueprintExtension* BPExtension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			FScopedTransaction Transaction = FScopedTransaction(INVTEXT("Removed View Model Assignment"));
			BPExtension->Modify();
			BPExtension->RemoveAssignment(*Item.Get());
		}
	}
}
