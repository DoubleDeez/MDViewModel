#include "Util/MDViewModelAssignmentReference.h"

#include "Serialization/CompactBinaryWriter.h"
#include "Util/MDViewModelAssignment.h"
#include "ViewModel/MDViewModelBase.h"

FMDViewModelAssignmentReference::FMDViewModelAssignmentReference(const FMDViewModelAssignment& Assignment)
	: ViewModelClass(Assignment.ViewModelClass)
	, ViewModelName(Assignment.ViewModelName)
{
}

FMDViewModelAssignmentReference::FMDViewModelAssignmentReference(TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
	: ViewModelClass(ViewModelClass)
	, ViewModelName(ViewModelName)
{
}

bool FMDViewModelAssignmentReference::IsAssignmentValid() const
{
	return !ViewModelClass.IsNull() && ViewModelName != NAME_None;
}

void FMDViewModelAssignmentReference::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsSaving() && IsAssignmentValid())
	{
		// Save broad and specific references
		Ar.MarkSearchableName(FMDViewModelAssignment::StaticStruct(), *ViewModelClass.GetAssetName());
		Ar.MarkSearchableName(FMDViewModelAssignment::StaticStruct(), *FString::Printf(TEXT("%s.%s"), *ViewModelClass.GetAssetName(), *ViewModelName.ToString()));
	}
}

FMDViewModelAssignmentReference& FMDViewModelAssignmentReference::operator=(const FMDViewModelAssignmentReference& Other)
{
	ViewModelClass = Other.ViewModelClass;
	ViewModelName = Other.ViewModelName;

#if WITH_EDITOR
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	OnGetWidgetClass = Other.OnGetWidgetClass;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	OnGetBoundObjectClass = Other.OnGetBoundObjectClass;
#endif

	return *this;
}

bool FMDViewModelAssignmentReference::operator==(const FMDViewModelAssignmentReference& Other) const
{
	return ViewModelClass == Other.ViewModelClass && ViewModelName == Other.ViewModelName;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelAssignmentReference& Assignment)
{
	Writer.BeginObject();
	Writer << "Class" << Assignment.ViewModelClass.ToString();
	Writer << "Name" << Assignment.ViewModelName;
	Writer.EndObject();
	return Writer;
}
