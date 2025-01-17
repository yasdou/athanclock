// #include "update.h"
// #include <iostream>
// #include <curl/curl.h>  // Zum Abrufen der GitHub API
// #include <json/json.h>  // Zum Parsen der JSON-Antwort von GitHub

// // Diese Funktion wird verwendet, um die JSON-Antwort von GitHub zu verarbeiten
// size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
//     ((std::string*)userp)->append((char*)contents, size * nmemb);
//     return size * nmemb;
// }

// // Funktion zum Überprüfen auf eine neue Version
// bool checkForUpdates(const std::string& currentVersion) {
//     CURL* curl;
//     CURLcode res;
//     std::string readBuffer;

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl = curl_easy_init();

//     if (curl) {
//         // GitHub API URL für das neueste Release des Repositories
//         curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/yasdou/athanclock/releases/latest");
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
//         curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");  // GitHub API benötigt einen User-Agent

//         res = curl_easy_perform(curl);

//         if (res != CURLE_OK) {
//             std::cerr << "cURL-Fehler: " << curl_easy_strerror(res) << std::endl;
//             curl_easy_cleanup(curl);
//             return false;
//         }

//         curl_easy_cleanup(curl);

//         // JSON-Antwort parsen
//         Json::Value root;
//         Json::CharReaderBuilder reader;
//         std::istringstream sstream(readBuffer);
//         std::string errs;

//         if (!Json::parseFromStream(reader, sstream, &root, &errs)) {
//             std::cerr << "Fehler beim Parsen der JSON-Antwort: " << errs << std::endl;
//             return false;
//         }

//         std::string latestVersion = root["tag_name"].asString();

//         std::cout << "Aktuelle Version: " << currentVersion << std::endl;
//         std::cout << "Neueste Version: " << latestVersion << std::endl;

//         // Versionen vergleichen
//         if (latestVersion != currentVersion) {
//             std::cout << "Es ist eine neue Version verfügbar!" << std::endl;
//             performOTAUpdate(latestVersion);
//             return true;
//         }
//     }
//     return false;
// }

// // Funktion zum Starten des OTA-Updates
// void performOTAUpdate(const std::string& newVersion) {
//     std::cout << "Starte OTA-Update auf Version: " << newVersion << std::endl;
//     // Hier würdest du den Code hinzufügen, um die neue Version herunterzuladen und zu installieren.
//     // Dies könnte das Herunterladen einer Binärdatei oder eines Archivs sein,
//     // und anschließend das Entpacken und Installieren der neuen Version.
// }
