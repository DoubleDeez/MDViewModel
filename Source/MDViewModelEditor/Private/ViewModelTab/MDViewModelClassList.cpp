#include "ViewModelTab/MDViewModelClassList.h"

#include "EditorClassUtils.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Styling/SlateIconFinder.h"
#include "UObject/UObjectIterator.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Layout/SScrollBorder.h"

class SClassItem : public STableRow< TSharedPtr<FString> >
{
public:

	SLATE_BEGIN_ARGS( SClassItem )
		{}
		/** The text this item should highlight, if any. */
		SLATE_ARGUMENT( FText, HighlightText )
		/** The node this item is associated with. */
		SLATE_ARGUMENT( TSharedPtr<FMDViewModelClassItem>, ClassItem)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		TSharedPtr<FMDViewModelClassItem> ClassItem = InArgs._ClassItem;

		const TSharedPtr<SToolTip> ToolTip = [&ClassItem]() -> TSharedPtr<SToolTip>
		{
			if (const UClass* Class = ClassItem->Class.Get())
			{
				UPackage*  Package  = Class->GetOutermost();
				return FEditorClassUtils::GetTooltip(Class);
			}
			else if (!ClassItem->ClassPath.IsNull())
			{
				return SNew(SToolTip).Text(FText::FromString(ClassItem->ClassPath.ToString()));
			}

			return nullptr;
		}();

		const FSlateBrush* ClassIcon = FSlateIconFinder::FindIconBrushForClass(ClassItem->Class.Get());

		this->ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 2.0f, 6.0f, 2.0f)
			[
				SNew(SImage)
				.Image(ClassIcon )
				.Visibility(ClassIcon != FAppStyle::GetDefaultBrush()? EVisibility::Visible : EVisibility::Collapsed)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding( 0.0f, 3.0f, 6.0f, 3.0f )
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ClassItem->DisplayName)
				.HighlightText(InArgs._HighlightText)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.ToolTip(ToolTip)
			]
		];

		STableRow<TSharedPtr<FString>>::ConstructInternal(STableRow::FArguments().ShowSelection(true), InOwnerTableView);
	}
};


void SMDViewModelClassList::Construct(const FArguments& InArgs, const TSharedPtr<FMDViewModelProviderBase>& Provider)
{
	SelectedClass = InArgs._SelectedClass;

	PopulateClassItems(Provider);

	const TSharedRef<SListView<TSharedPtr<FMDViewModelClassItem>>> ClassListView = SAssignNew(ClassList, SListView<TSharedPtr<FMDViewModelClassItem>>)
	    .SelectionMode(ESelectionMode::Single)
	    .ListItemsSource(&ClassItems)
	    .OnGenerateRow(this, &SMDViewModelClassList::OnGenerateRowForClassViewer)
	    .OnSelectionChanged(InArgs._OnSelectionChanged)
	    .ItemHeight(20.0f)
	    .HeaderRow
	    (
    		SNew(SHeaderRow)
    		.Visibility(EVisibility::Collapsed)
    		+ SHeaderRow::Column(TEXT("Class"))
    		.DefaultLabel(NSLOCTEXT("ClassViewer", "Class", "Class"))
	    );

	if (SelectedItem.IsValid())
	{
		ClassListView->SetSelection(SelectedItem);
	}

	ChildSlot
	[
		SNew(SScrollBorder, ClassListView)
		[
			ClassListView
		]
	];
}

TSharedRef<ITableRow> SMDViewModelClassList::OnGenerateRowForClassViewer(TSharedPtr<FMDViewModelClassItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	// TODO - Add search support
	// If the item passed the filter, it may be from a match with hidden class name strings, update the search box text to retain highlighting of valid matching text
	/*FText SearchBoxTextForHighlight = SearchBox->GetText();
	if (!SearchBoxTextForHighlight.IsEmpty() && (ClassNameDisplay->Find(*SearchBoxTextForHighlight.ToString()) == INDEX_NONE))
	{
		SearchBoxTextForHighlight = FText::FromString(UObjectBase::RemoveClassPrefix(*SearchBoxTextForHighlight.ToString()));
	}*/

	return SNew(SClassItem, OwnerTable)
		//.HighlightText(SearchBoxTextForHighlight)
		.ClassItem(Item);
}

void SMDViewModelClassList::PopulateClassItems(const TSharedPtr<FMDViewModelProviderBase>& Provider)
{
	TMap<FTopLevelAssetPath, TSharedPtr<FMDViewModelClassItem>> ClassPathToNode;

	// Create a node for every Blueprint class listed in the AssetRegistry and set the Blueprint fields
	// Retrieve all blueprint classes
	{
		auto CreateUnloadedClassItem = [](TSharedPtr<FMDViewModelClassItem>& InOutClassItem, const FAssetData& InAssetData, FTopLevelAssetPath InClassPath)
		{
			if (!InOutClassItem)
			{
				const FString ClassName = InAssetData.AssetName.ToString();
				FString ClassDisplayName = InAssetData.GetTagValueRef<FString>(FBlueprintTags::BlueprintDisplayName);
				if (ClassDisplayName.IsEmpty())
				{
					ClassDisplayName = ClassName;
				}
				InOutClassItem = MakeShared<FMDViewModelClassItem>();
			}

			/*if (InOutClassItem->UnloadedBlueprintData.IsValid())
			{
				// Already set
				return;
			}*/

			InOutClassItem->ClassPath = InClassPath;
			if (InOutClassItem->ParentClassPath.IsNull())
			{
				FString ParentClassPathString;
				if (InAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPathString))
				{
					InOutClassItem->ParentClassPath = FTopLevelAssetPath(*FPackageName::ExportTextPathToObjectPath(ParentClassPathString));
				}
			}

			//InOutClassItem->BlueprintAssetPath = InAssetData.GetSoftObjectPath();
		};

		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
		FString ClassPathString;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses=*/true);

		for (const FAssetData& AssetData : Assets)
		{
			const FTopLevelAssetPath ClassPath = FEditorClassUtils::GetClassPathNameFromAssetTag(AssetData);
			if (!ClassPath.IsNull())
			{
				TSharedPtr<FMDViewModelClassItem>& Node = ClassPathToNode.FindOrAdd(ClassPath);
				CreateUnloadedClassItem(Node, AssetData, ClassPath);
			}
		}

		Assets.Reset();
		AssetRegistry.GetAssetsByClass(UBlueprintGeneratedClass::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses=*/true);

		for (const FAssetData& AssetData : Assets)
		{
			FTopLevelAssetPath ClassPathNameFromAssetPath = AssetData.GetSoftObjectPath().GetAssetPath();
			TSharedPtr<FMDViewModelClassItem>& Node = ClassPathToNode.FindOrAdd(ClassPathNameFromAssetPath);
			CreateUnloadedClassItem(Node, AssetData, ClassPathNameFromAssetPath);
		}
	}

	// FindOrCreate a node for every loaded UClass, and set the UClass fields
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* CurrentClass = *ClassIt;
		// Ignore deprecated and temporary trash classes.
		if (CurrentClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden) ||
			FKismetEditorUtilities::IsClassABlueprintSkeleton(CurrentClass))
		{
			continue;
		}

		TSharedPtr<FMDViewModelClassItem>& Node = ClassPathToNode.FindOrAdd(CurrentClass->GetClassPathName());
		if (!Node)
		{
			Node = MakeShared<FMDViewModelClassItem>();
		}

		if (!Node->Class.IsValid())
		{
			Node->Class = CurrentClass;
			Node->DisplayName = CurrentClass->GetDisplayNameText();
			Node->ClassPath = CurrentClass->GetClassPathName();
			//Node->Blueprint = Cast<UBlueprint>(CurrentClass->ClassGeneratedBy);
			if (const UClass* SuperClass = CurrentClass->GetSuperClass())
			{
				Node->ParentClassPath = SuperClass->GetClassPathName();
			}
		}

		if (CurrentClass == SelectedClass)
		{
			SelectedItem = Node;
		}
	}

	for (TPair<FTopLevelAssetPath, TSharedPtr<FMDViewModelClassItem>>& KVPair : ClassPathToNode)
	{
		TSharedPtr<FMDViewModelClassItem>& Node = KVPair.Value;
		if (Node->Class == UObject::StaticClass())
		{
			// No parent expected for the root class
			continue;
		}

		TSharedPtr<FMDViewModelClassItem>* ParentNodePtr = nullptr;
		if (!Node->ParentClassPath.IsNull())
		{
			ParentNodePtr = ClassPathToNode.Find(Node->ParentClassPath);
		}

		if (!ParentNodePtr)
		{
			continue;
		}

		const TSharedPtr<FMDViewModelClassItem>& ParentNode = *ParentNodePtr;
		check(ParentNode);
		ParentNode->Children.Add(Node);
		Node->ParentItem = ParentNode;
	}

	ClassItems.Reset(ClassPathToNode.Num());

	ClassPathToNode.GenerateValueArray(ClassItems);

	TArray<FMDViewModelSupportedClass> ViewModelClasses;
	Provider->GetSupportedViewModelClasses(ViewModelClasses);
	ClassItems.RemoveAll([&ViewModelClasses](const TSharedPtr<FMDViewModelClassItem>& Item)
	{
		if (Item.IsValid())
		{
			const UClass* Class = Item->Class.Get();
			if (Class != nullptr && Class->HasAnyClassFlags(CLASS_Abstract))
			{
				return true;
			}

			for (const FMDViewModelSupportedClass& SupportedClass : ViewModelClasses)
			{
				if (Class != nullptr)
				{
					if (SupportedClass.Class == Class || (SupportedClass.bAllowChildClasses && Class->IsChildOf(SupportedClass.Class)))
					{
						return false;
					}
				}
				else
				{
					// If SupportedClass.Class == Item, then the class would be loaded and we wouldn't be in this code path, so only check the IsChildOf case
					if (SupportedClass.bAllowChildClasses)
					{
						TSharedPtr<FMDViewModelClassItem> CurrentItem = Item->ParentItem.Pin();

						// Keep going through parents till you find an invalid.
						while (CurrentItem.IsValid())
						{
							if (CurrentItem->Class.Get() == SupportedClass.Class)
							{
								return false;
							}

							CurrentItem = CurrentItem->ParentItem.Pin();
						}
					}
				}
			}
		}

		return true;
	});
}
