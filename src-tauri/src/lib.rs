use tauri::Manager;

mod commands;
mod models;
mod cura_state;
mod scanner;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_shell::init())
        .manage(cura_state::CuraState::new())
        .setup(|app| {
            #[cfg(debug_assertions)]
            {
                let window = app.get_webview_window("main").unwrap();
                window.open_devtools();
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::cura_scan::get_version,
            commands::cura_scan::select_folders,
            commands::cura_scan::start_scan,
            commands::cura_scan::cancel_scan,
            commands::cura_duplicates::find_duplicates,
            commands::cura_duplicates::delete_file,
            commands::cura_duplicates::move_file,
            commands::cura_organize::preview_organize,
            commands::cura_organize::execute_organize,
            commands::cura_organize::select_organize_destination,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}