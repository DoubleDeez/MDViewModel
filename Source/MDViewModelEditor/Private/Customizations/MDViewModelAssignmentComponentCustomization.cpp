#include "Customizations/MDViewModelAssignmentComponentCustomization.h"

#include "Components/MDViewModelAssignmentComponent.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Util/MDVMEditorUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace MDVMACC
{
	UObject* GetTemplateObject(UMDViewModelAssignmentComponent& Component)
	{
		if (Component.IsTemplate())
		{
			return &Component;
		}

		if (IMDVMCompiledAssignmentsInterface* AssignmentsInterface = MDViewModelUtils::GetCompiledAssignmentsInterface(Component.GetOwningObjectClass()))
		{
			return Cast<UObject>(AssignmentsInterface);
		}

		return nullptr;
	}
}

class FMDVMAssignmentViewModelBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FMDVMAssignmentViewModelBuilder>
{
public:
	FMDVMAssignmentViewModelBuilder(const TSharedPtr<IPropertyHandle>& VMHandle, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& AssignmentData)
		: VMHandle(VMHandle)
		, Assignment(Assignment)
		, AssignmentData(AssignmentData)
	{}

	virtual FName GetName() const override
	{
		const TSubclassOf<UMDViewModelBase> VMClass = Assignment.ViewModelClass;
		const FText VMClassName = IsValid(VMClass) ? VMClass->GetDisplayNameText() : INVTEXT("[Invalid Assignment]");
		const FString AssignmentName = FString::Printf(TEXT("%s (%s)"), *VMClassName.ToString(), *Assignment.ViewModelName.ToString());
		return *AssignmentName;
	}

	virtual bool InitiallyCollapsed() const override { return true; }

	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const override
	{
		return VMHandle;
	}

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
	{
		const TSubclassOf<UMDViewModelBase> VMClass = Assignment.ViewModelClass;
		const FText VMClassName = IsValid(VMClass) ? VMClass->GetDisplayNameText() : INVTEXT("[Invalid Assignment]");
		const FText AssignmentName = FText::Format(INVTEXT("{0} ({1})"), VMClassName, FText::FromName(Assignment.ViewModelName));
		const FText VMName = FText::FromString(GetNameSafe(GetViewModel()));

		const FText Tooltip = [&]()
		{
			const FText ViewModelText = IsValid(VMClass)
				? VMClass->GetToolTipText()
				: INVTEXT("Invalid View Model Class");

			const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Assignment.ProviderTag);
			const FText ProviderText = IsValid(Provider)
				? FText::Format(INVTEXT("Provider: {0}\n{1}"), Provider->GetDisplayName(), Provider->GetDescription(AssignmentData.ProviderSettings))
				: INVTEXT("Invalid Provider");

			return FText::Format(INVTEXT("{0}\n\n{1}"), ViewModelText, ProviderText);
		}();

		NodeRow
		.OverrideResetToDefault(FResetToDefaultOverride::Hide(true))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(AssignmentName)
			.ToolTipText(Tooltip)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(VMName)
			.ToolTipText(VMName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
	}

	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
	{
		UMDViewModelBase* ViewModel = GetViewModel();
		if (IsValid(ViewModel) && Assignment.IsValid())
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			const FAddPropertyParams Params = FAddPropertyParams()
				.ForceShowProperty()
				.AllowChildren(true)
				.CreateCategoryNodes(false)
				.UniqueId(*FString::Printf(TEXT("%s - %s"), *Assignment.ViewModelClass->GetDisplayNameText().ToString(), *Assignment.ViewModelName.ToString()));

			IDetailPropertyRow* ChildRow = ChildrenBuilder.AddExternalObjects({ ViewModel }, Params);
			if (ChildRow != nullptr)
			{
				ChildRow->Visibility(EVisibility::Collapsed);
				if (const TSharedPtr<IPropertyHandle> Handle = ChildRow->GetPropertyHandle())
				{
					uint32 NumChildren = 0;
					Handle->GetNumChildren(NumChildren);

					for (uint32 i = 0; i < NumChildren; ++i)
					{
						if (const TSharedPtr<IPropertyHandle> Child = Handle->GetChildHandle(i))
						{
							const FProperty* Property = Child->GetProperty();
							if (Property != nullptr && !Property->HasMetaData(FMDViewModelGraphStatics::VMHiddenMeta))
							{
								const bool bCanEdit = Property->HasAnyPropertyFlags(CPF_Edit);
								ChildrenBuilder
									.AddProperty(Child.ToSharedRef())
									.IsEnabled(bCanEdit)
									.EditCondition(bCanEdit, {});
							}
						}
					}
				}
			}
#else
			for (TFieldIterator<FProperty> It(ViewModel->GetClass()); It; ++It)
			{
				const FProperty* Prop = *It;
				if (Prop != nullptr && !Prop->HasMetaData(FMDViewModelGraphStatics::VMHiddenMeta))
				{
					const FAddPropertyParams Params = FAddPropertyParams()
						.ForceShowProperty()
						.AllowChildren(true)
						.CreateCategoryNodes(false);
					if (IDetailPropertyRow* ChildRow = ChildrenBuilder.AddExternalObjectProperty({ ViewModel }, Prop->GetFName(), Params))
					{
						const bool bCanEdit = Prop->HasAnyPropertyFlags(CPF_Edit);
						ChildRow->IsEnabled(bCanEdit).EditCondition(bCanEdit, {});
					}
				}
			}
#endif
		}
	}

private:
	UMDViewModelBase* GetViewModel() const
	{
		if (VMHandle.IsValid())
		{
			UObject* VMObject = nullptr;
			if (VMHandle->GetValue(VMObject) == FPropertyAccess::Success)
			{
				return Cast<UMDViewModelBase>(VMObject);
			}
		}

		return nullptr;
	}

	const TSharedPtr<IPropertyHandle> VMHandle;
	const FMDViewModelAssignment Assignment;
	const FMDViewModelAssignmentData AssignmentData;
};

class FMDVMAssignmentsBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FMDVMAssignmentsBuilder>
{
public:
	FMDVMAssignmentsBuilder(const TSharedRef<IPropertyHandle>& AssignmentsHandle, const TSharedRef<IPropertyHandle>& ViewModelsHandle)
		: AssignmentsHandle(AssignmentsHandle)
		, ViewModelsHandle(ViewModelsHandle)
	{}

	virtual FName GetName() const override { return TEXT("MDVMAssignments"); }
	virtual bool InitiallyCollapsed() const override { return false; }

	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const override
	{
		return AssignmentsHandle;
	}

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override
	{
		NodeRow
		.OverrideResetToDefault(FResetToDefaultOverride::Hide(true))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(INVTEXT("View Models"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(this, &FMDVMAssignmentsBuilder::GetValueText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
	}

	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
	{
		uint32 NumEntries = 0;
		AssignmentsHandle->GetNumChildren(NumEntries);
		for (uint32 EntryIndex = 0; EntryIndex < NumEntries; ++EntryIndex)
		{
			TSharedPtr<IPropertyHandle> ValueHandle = AssignmentsHandle->GetChildHandle(EntryIndex);
			TSharedPtr<IPropertyHandle> KeyHandle = ValueHandle.IsValid() ? ValueHandle->GetKeyHandle() : nullptr;
			if (!KeyHandle.IsValid())
			{
				continue;
			}

			void* ValuePtr = nullptr;
			if (ValueHandle->GetValueData(ValuePtr) != FPropertyAccess::Success || ValuePtr == nullptr)
			{
				continue;
			}

			void* KeyPtr = nullptr;
			if (KeyHandle->GetValueData(KeyPtr) != FPropertyAccess::Success || KeyPtr == nullptr)
			{
				continue;
			}

			const FMDViewModelAssignment& Assignment = *static_cast<FMDViewModelAssignment*>(KeyPtr);
			const FMDViewModelAssignmentData& AssignmentData = *static_cast<FMDViewModelAssignmentData*>(ValuePtr);

			const TSharedPtr<IPropertyHandle> VMHandle = FindViewModelHandle(Assignment);
			ChildrenBuilder.AddCustomBuilder(MakeShared<FMDVMAssignmentViewModelBuilder>(VMHandle, Assignment, AssignmentData));
		}
	}

private:
	FText GetValueText() const
	{
		uint32 NumElements = 0;
		AssignmentsHandle->AsMap()->GetNumElements(NumElements);
		return FText::FormatNamed(INVTEXT("{Num} {Num}|plural(one=\"Assignment\",other=\"Assignments\")"), TEXT("Num"), NumElements);
	}

	TSharedPtr<IPropertyHandle> FindViewModelHandle(const FMDViewModelAssignment& Assignment) const
	{
		const FMDViewModelAssignmentReference SearchReference = FMDViewModelAssignmentReference(Assignment);
		uint32 NumEntries = 0;
		ViewModelsHandle->GetNumChildren(NumEntries);
		for (uint32 EntryIndex = 0; EntryIndex < NumEntries; ++EntryIndex)
		{
			TSharedPtr<IPropertyHandle> ValueHandle = ViewModelsHandle->GetChildHandle(EntryIndex);
			TSharedPtr<IPropertyHandle> KeyHandle = ValueHandle.IsValid() ? ValueHandle->GetKeyHandle() : nullptr;
			if (!KeyHandle.IsValid())
			{
				continue;
			}

			void* DataPtr = nullptr;
			if (KeyHandle->GetValueData(DataPtr) != FPropertyAccess::Success || DataPtr == nullptr)
			{
				continue;
			}

			const FMDViewModelAssignmentReference& AssignmentReference = *static_cast<FMDViewModelAssignmentReference*>(DataPtr);
			if (AssignmentReference == SearchReference)
			{
				return ValueHandle;
			}
		}

		return nullptr;
	}

	const TSharedRef<IPropertyHandle> AssignmentsHandle;
	const TSharedRef<IPropertyHandle> ViewModelsHandle;
};

void FMDViewModelAssignmentComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	HideDefaultCategories();
	TryAddViewModelEditorButton();
	AddViewModelDetails();
}

void FMDViewModelAssignmentComponentCustomization::HideDefaultCategories()
{
	const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = CachedDetailBuilder.Pin();
	if (!DetailBuilder.IsValid())
	{
		return;
	}

	TArray<FName> Categories;
	DetailBuilder->GetCategoryNames(Categories);
	for (const FName& Category : Categories)
	{
		DetailBuilder->HideCategory(Category);
	}
}

void FMDViewModelAssignmentComponentCustomization::TryAddViewModelEditorButton()
{
	const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = CachedDetailBuilder.Pin();
	if (!DetailBuilder.IsValid())
	{
		return;
	}

	const TSharedPtr<FTabManager> HostTabManager = DetailBuilder->GetDetailsView()->GetHostTabManager();
	if (!HostTabManager.IsValid() || !HostTabManager->HasTabSpawner(FMDViewModelSummoner::TabID))
	{
		return;
	}

	DetailBuilder->EditCategory("ViewModelEditor", INVTEXT("View Model Editor"), ECategoryPriority::Important)
		.AddCustomRow(INVTEXT("View Model Editor"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(INVTEXT("View Model Editor"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SButton)
			.OnClicked(this, &FMDViewModelAssignmentComponentCustomization::OpenViewModelEditorTab)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Open View Model Editor"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
}

void FMDViewModelAssignmentComponentCustomization::AddViewModelDetails()
{
	const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = CachedDetailBuilder.Pin();
	if (!DetailBuilder.IsValid())
	{
		return;
	}

	UMDViewModelAssignmentComponent* Component = GetComponent();
	if (!IsValid(Component))
	{
		return;
	}

	IDetailCategoryBuilder& AssignmentCategory = DetailBuilder->EditCategory("ViewModelAssignments", INVTEXT("View Model Assignments"));
	TSharedRef<IPropertyHandle> ViewModelsHandle = DetailBuilder->GetProperty(TEXT("ViewModels"), UMDViewModelAssignmentComponent::StaticClass());
	TSharedPtr<IPropertyHandle> AssignmentsHandle = nullptr;

	if (Component->IsTemplate())
	{
		AssignmentsHandle = DetailBuilder->GetProperty(TEXT("Assignments"), UMDViewModelAssignmentComponent::StaticClass());
	}
	else if (IDetailPropertyRow* Row = AssignmentCategory.AddExternalObjectProperty({ MDVMACC::GetTemplateObject(*Component) }, TEXT("Assignments")))
	{
		// Our custom builder will be displaying the assignments, but we need to add the property to have a property handle
		Row->Visibility(EVisibility::Collapsed);
		AssignmentsHandle = Row->GetPropertyHandle();
	}

	if (!AssignmentsHandle.IsValid())
	{
		return;
	}

	AssignmentCategory.AddCustomBuilder(MakeShared<FMDVMAssignmentsBuilder>(AssignmentsHandle.ToSharedRef(), ViewModelsHandle));

	Component->StopListeningForAnyViewModelChanged(this);
	Component->ListenForAnyViewModelChanged(FSimpleDelegate::CreateSP(this, &FMDViewModelAssignmentComponentCustomization::RequestRefresh));
}

FReply FMDViewModelAssignmentComponentCustomization::OpenViewModelEditorTab() const
{
	if (const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = CachedDetailBuilder.Pin())
	{
		if (const TSharedPtr<FTabManager> HostTabManager = DetailBuilder->GetDetailsView()->GetHostTabManager())
		{
			if (HostTabManager->HasTabSpawner(FMDViewModelSummoner::TabID))
			{
				HostTabManager->TryInvokeTab(FMDViewModelSummoner::TabID);
			}
		}
	}

	return FReply::Handled();
}

UMDViewModelAssignmentComponent* FMDViewModelAssignmentComponentCustomization::GetComponent() const
{
	TArray<UMDViewModelAssignmentComponent*> Components;
	if (const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = CachedDetailBuilder.Pin())
	{
		TArray<TWeakObjectPtr<UObject>> WeakComponents;
		DetailBuilder->GetObjectsBeingCustomized(WeakComponents);

		// Only support single selection
		if (WeakComponents.Num() == 1 && WeakComponents[0].IsValid() && WeakComponents[0]->IsA<UMDViewModelAssignmentComponent>())
		{
			return Cast<UMDViewModelAssignmentComponent>(WeakComponents[0].Get());
		}
	}

	return nullptr;
}

void FMDViewModelAssignmentComponentCustomization::RequestRefresh()
{
	if (IDetailLayoutBuilder* DetailBuilder = CachedDetailBuilder.Pin().Get())
	{
		DetailBuilder->ForceRefreshDetails();
	}
}
