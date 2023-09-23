#include "Widgets/SMDVMConfigEditor.h"

#include "ClassViewerModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailsViewArgs.h"
#include "IDetailCustomization.h"
#include "MDViewModelEditorConfig.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Util/MDViewModelClassFilter.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Util/MDVMEditorUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"

class FMDVMConfigDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FMDVMConfigDetails>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder.GetObjectsBeingCustomized(Objects);

		if (Objects.IsEmpty())
		{
			return;
		}

		TArray<FName> Categories;
		DetailBuilder.GetCategoryNames(Categories);

		IDetailCategoryBuilder& ConfigCategory = DetailBuilder.EditCategory(TEXT("View Model Config Properties"));

		for (const FName& CategoryName : Categories)
		{
			TArray<TSharedRef<IPropertyHandle>> Properties;
			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName);
			Category.GetDefaultProperties(Properties);
			Category.SetCategoryVisibility(false);
			DetailBuilder.HideCategory(CategoryName);
			for (const TSharedRef<IPropertyHandle>& Property : Properties)
			{
				const FProperty* Prop = Property->GetProperty();
				if (Prop != nullptr && !Property->HasMetaData(FMDViewModelGraphStatics::VMHiddenMeta) && Prop->HasAnyPropertyFlags(CPF_Config))
				{
					IDetailPropertyRow& Row = ConfigCategory.AddProperty(Property);
					if (Prop->HasAnyPropertyFlags(CPF_GlobalConfig) && Prop->GetOwnerClass() != Objects[0].Get())
					{
						Row.IsEnabled(false).EditCondition(false, {});
					}
				}
			}
		}
	}
};

void SMDVMConfigEditor::Construct(const FArguments& InArgs)
{
	FClassViewerInitializationOptions ClassPickerOptions;
	ClassPickerOptions.bShowNoneOption = false;
	ClassPickerOptions.bShowUnloadedBlueprints = true;
	ClassPickerOptions.bExpandAllNodes = true;
	ClassPickerOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
	ClassPickerOptions.ClassFilters.Add(MakeShared<FMDViewModelClassFilter>(true));
	ClassPickerOptions.NameTypeToDisplay = GetDefault<UMDViewModelEditorConfig>()->GetNameTypeToDisplay();

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	const TSharedRef<SWidget> ClassViewer = ClassViewerModule.CreateClassViewer(ClassPickerOptions, FOnClassPicked::CreateSP(this, &SMDVMConfigEditor::OnClassPicked));

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.bShowScrollBar = true;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.bForceHiddenPropertyVisibility = true;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	ViewModelDetails = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	ViewModelDetails->RegisterInstancedCustomPropertyLayout(UMDViewModelBase::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FMDVMConfigDetails::MakeInstance));

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(400.f)
			[
				ClassViewer
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			ViewModelDetails.ToSharedRef()
		]
	];
}

void SMDVMConfigEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		if (PropertyChangedEvent.GetNumObjectsBeingEdited() > 0)
		{
			UMDViewModelBase* ViewModelCDO = Cast<UMDViewModelBase>(const_cast<UObject*>(PropertyChangedEvent.GetObjectBeingEdited(0)));
			MDVMEditorUtils::SaveViewModelConfig(ViewModelCDO);
		}
	}
}

void SMDVMConfigEditor::OnClassPicked(UClass* Class)
{
	if (ViewModelDetails.IsValid() && IsValid(Class))
	{
		ViewModelDetails->SetObject(Class->GetDefaultObject());
	}
}
