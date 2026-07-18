import os

import unreal


PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CSV_PATH = os.path.join(PROJECT_ROOT, "SourceAssets", "IdolQuiz", "IdolQuizQuestions.csv")
DATA_DIR = "/Game/IdolQuiz/Data"
TABLE_PATH = DATA_DIR + "/DT_IdolQuizQuestions"


def main():
    if not os.path.isfile(CSV_PATH):
        raise RuntimeError("Idol Quiz CSV is missing: {}".format(CSV_PATH))

    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        factory = unreal.DataTableFactory()
        row_struct = unreal.load_object(None, "/Script/AllGames.IdolQuizQuestion")
        if row_struct is None:
            raise RuntimeError("Failed to load IdolQuizQuestion ScriptStruct")
        factory.set_editor_property("struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DT_IdolQuizQuestions", DATA_DIR, unreal.DataTable, factory
        )
    if table is None:
        raise RuntimeError("Failed to create Idol Quiz DataTable")

    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, CSV_PATH):
        raise RuntimeError("Failed to import Idol Quiz CSV")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)

    rows = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    if len(rows) != 96:
        raise RuntimeError("Expected 96 DataTable rows, found {}".format(len(rows)))
    unreal.log("IDOL QUIZ DATA TABLE READY: {} rows from {}".format(len(rows), CSV_PATH))


main()
