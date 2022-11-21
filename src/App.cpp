#include "pch.h"
#include "App.h"
#include <Windows.h>
#include "CommDlg.h"
#define WIN32_LEAN_AND_MEAN 

namespace fs = std::filesystem;

const auto version = "0.2.4";

extern string datapath;

const auto api_script = R"(
const BETTERNCM_API_PATH="http://localhost:3248/api"
const BETTERNCM_FILES_PATH="http://localhost:3248/local"

const betterncm={
    fs:{
        async readDir(path){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/read_dir?path="+encodeURIComponent(path),{headers:{BETTERNCM_API_KEY}})).json() 
        },
        async readFileText(path){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/read_file_text?path="+encodeURIComponent(path),{headers:{BETTERNCM_API_KEY}})).text() 
        },
		async unzip(path,dist=path+"_extracted/"){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/unzip_file?path="+encodeURIComponent(path)+"&dist="+encodeURIComponent(dist),{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async writeFileText(path,content){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/write_file_text?path="+encodeURIComponent(path),{method:"POST",body:content,headers:{BETTERNCM_API_KEY}})).text() 
        },
		async writeFile(path,file){
			let fd=new FormData()
			fd.append("file",file)
			await fetch(BETTERNCM_API_PATH+"/fs/write_file?path="+encodeURIComponent(path), {
				  method: 'POST',
				  body: fd,
				  headers:{BETTERNCM_API_KEY}
			});
        },
        async mkdir(path){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/mkdir?path="+encodeURIComponent(path),{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async exists(path){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/exists?path="+encodeURIComponent(path),{headers:{BETTERNCM_API_KEY}})).json() 
        },
        async remove(path){
           return await(await fetch(BETTERNCM_API_PATH+"/fs/remove?path="+encodeURIComponent(path),{headers:{BETTERNCM_API_KEY}})).text() 
        },
    },app:{
        async exec(cmd,ele=false, showWindow=false){
           return await(await fetch(BETTERNCM_API_PATH+"/app/exec"+(ele?"_ele":"")+(showWindow?"?_showWindow":""),{method:"POST",body:cmd, headers:{BETTERNCM_API_KEY}})).text() 
        },
        async getBetterNCMVersion(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/version",{headers:{BETTERNCM_API_KEY}})).text() 
        },
		async takeBackgroundScreenshot(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/bg_screenshot",{headers:{BETTERNCM_API_KEY}})).text() 
        },
		async getNCMWinPos(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/get_win_position")).json() 
        },
		async reloadPlugins(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/reload_plugin",{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async getDataPath(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/datapath",{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async readConfig(key,def){
            return await(await fetch(BETTERNCM_API_PATH+"/app/read_config?key="+key+"&default="+def,{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async writeConfig(key,value){
            return await(await fetch(BETTERNCM_API_PATH+"/app/write_config?key="+key+"&value="+value,{headers:{BETTERNCM_API_KEY}})).text() 
        },
        async getNCMPath(){
            return await(await fetch(BETTERNCM_API_PATH+"/app/ncmpath",{headers:{BETTERNCM_API_KEY}})).text() 
        },
		async showConsole(){
			return await(await fetch(BETTERNCM_API_PATH+"/app/show_console",{headers:{BETTERNCM_API_KEY}})).text() 
		},
		async openFileDialog(filter,initialDir){
			if(initialDir==undefined)initialDir=await betterncm.app.getDataPath();
            return await(await fetch(BETTERNCM_API_PATH+"/app/open_file_dialog?filter="+encodeURIComponent(filter)+"&initialDir="+encodeURIComponent(initialDir),{headers:{BETTERNCM_API_KEY}})).text() 
        }

    },ncm:{
        findNativeFunction(obj, identifiers) {
            for (var key in obj) {
                var flag = true;
                for (var _i = 0, identifiers_1 = identifiers; _i < identifiers_1.length; _i++) {
                    var identifier = identifiers_1[_i];
                    if (!obj[key].toString().includes(identifier))
                        flag = false;
                }
                if (flag)
                    return key;
            }
        },
        getPlaying(){
            if(document.querySelector(".btn-fav")){
                return { type: "normal", id: document.querySelector(".btn-fav").dataset.resId, title: document.querySelector(".title").innerText}
            }
            if(document.querySelector(".m-player-fm")){
                return { type:"fm", id: document.querySelector("button[data-action=hate]").dataset.resId, title: document.querySelector(".title").innerText}
            }
        }
    },
	utils:{
		waitForElement(selector,interval=100){
			return betterncm.utils.waitForFunction(()=>document.querySelector(selector));
		},waitForFunction(func,interval=100){
			return new Promise((rs)=>{
				let handle=setInterval(()=>{
					let result=func();
					if(result){
						clearInterval(handle);
						rs(result)
					}
				},interval)
			})
		},delay(ms){
		return new Promise((rs)=>setTimeout(rs,ms));
		}
	}
}
)";

const auto loader_script = R"(
async function loadPlugins() {
    const loadedPlugins = {}

    const AsyncFunction = (async function () { }).constructor;
    const PageMap = {
        "/pub/app.html": "Main"
    }
    const PageName = PageMap[location.pathname]

    async function loadPlugin(pluginPath, devMode = false) {
        async function loadInject(filePath, devMode, manifest) {
            let code = await betterncm.fs.readFileText(filePath)
            if (devMode) {
                setInterval(async () => {
                    if (code !== (await betterncm.fs.readFileText(filePath))) document.location.reload()
                }, 300)
            }

            if (filePath.endsWith('.js')) {
                let plugin = {
                    onLoad(fn) { this._load = fn },
                    onConfig(fn) { this._config = fn },
                    onAllPluginsLoaded(fn) { this._allLoaded = fn },
                    getConfig(configKey, defaul) {
                        let config = JSON.parse(localStorage["config.betterncm." + manifest.name] || "{}");
						if(config[configKey]!=undefined)return config[configKey]
                        return defaul;
                    },
                    setConfig(configKey, value) {
                        let config = JSON.parse(localStorage["config.betterncm." + manifest.name] || "{}");
                        config[configKey] = value;
                        localStorage["config.betterncm." + manifest.name] = JSON.stringify(config)
                    },
                    pluginPath
                }
                new AsyncFunction("plugin", code).call(loadedPlugins[manifest.name], plugin)
                await plugin._load?.call(loadedPlugins[manifest.name], plugin)
                loadedPlugins[manifest.name].injects.push(plugin);
            }
        }
        let manifest = JSON.parse(await betterncm.fs.readFileText(pluginPath + "/manifest.json"));

        loadedPlugins[manifest.name] = { pluginPath, manifest, finished: false, injects: [] }

        // Load Injects
        if (manifest.injects[PageName]) {
            for (let inject of manifest.injects[PageName]) {
                await (loadInject(pluginPath + "/" + inject.file, devMode, manifest));
            }
        }

		if (manifest.injects[location.pathname]) {
            for (let inject of manifest.injects[location.pathname]) {
                await (loadInject(pluginPath + "/" + inject.file, devMode, manifest));
            }
        }
    }

    let loadingPromises = []

    let pluginPaths = await betterncm.fs.readDir("./plugins_runtime");
    for (let path of pluginPaths) {
        loadingPromises.push(loadPlugin(path).then(_ => _).catch(e => { throw Error(`Failed to load plugin ${path}: ${e.toString()}`) }));
    }

    if (await betterncm.fs.exists("./plugins_dev")) {
        let devPluginPaths = await betterncm.fs.readDir("./plugins_dev");
        for (let path of devPluginPaths) {
            loadingPromises.push(loadPlugin(path, true).then(_ => _).catch(e => { console.error(`Failed to load dev plugin ${path}: ${e.toString()}`) }));
        }
    }


    await Promise.all(loadingPromises)
    window.loadedPlugins = loadedPlugins
    for (let name in loadedPlugins) loadedPlugins[name].injects.forEach(v => v._allLoaded?.call(loadedPlugins[name], loadedPlugins))
}

window.addEventListener("load",async ()=>{
	await loadPlugins();
	if(!("PluginMarket" in loadedPlugins)){
		let attempts=parseInt(localStorage["cc.microblock.loader.reloadPluginAttempts"]||"0");
		if(attempts<3){
			localStorage["cc.microblock.loader.reloadPluginAttempts"] = attempts+1;
			document.location.reload()
		}else{
			localStorage["cc.microblock.loader.reloadPluginAttempts"] = "0";
			alert("Failed to load plugins! Attempted for "+ attempts +" times");
		}
	}
});
)";

const auto plugin_manager_script = R"(
// dom create tool
function dom(tag, settings, ...children) {
    let tmp = document.createElement(tag);
    if (settings.class) {
        for (let cl of settings.class) {
            tmp.classList.add(cl)
        }
        delete settings.class
    }

    if (settings.style) {
        for (let cl in settings.style) {
            tmp.style[cl] = settings.style[cl]
        }
        delete settings.style
    }

    for (let v in settings) {
        tmp[v] = settings[v];
    }

    for (let child of children) {
        if (child)
            tmp.appendChild(child)
    }
    return tmp
}



betterncm.utils.waitForElement(".g-mn-set").then(async(settingsDom)=>{

 settingsDom.prepend(
        dom("div", { style: { marginLeft: "30px" } },
            dom("div", { style: { display: "flex", flexDirection: "column", alignItems: "center" } },
                dom("img", { src: "https://s1.ax1x.com/2022/08/11/vGlJN8.png", style: { width: "60px" } }),
                dom("div", { innerText: "BetterNCM II", style: { fontSize: "20px", fontWeight: "700" } }),
                dom("div", { innerText: "v" + await betterncm.app.getBetterNCMVersion() }),
                dom("div",{class:["BetterNCM-PM-Updatey"]}),
                dom("div", { style: { marginBottom: "20px" } },
                    dom("a", { class: ["u-ibtn5", "u-ibtnsz8"], innerText: "Open Folder", onclick: async () => { await betterncm.app.exec(`explorer "${(await betterncm.app.getDataPath()).replace(/\//g,"\\")}"`,false,true) }, style: { margin: "5px" } }),
					dom("a", { class: ["u-ibtn5", "u-ibtnsz8"], innerText: "Show Console", onclick: async () => { await betterncm.app.showConsole() }, style: { margin: "5px" } }),
					dom("a", { class: ["u-ibtn5", "u-ibtnsz8"], innerText: "Reload Plugins", onclick: async () => { await betterncm.app.reloadPlugins();document.location.reload(); }, style: { margin: "5px" } }),
                )
            ),
            dom("div", { class: ["BetterNCM-Plugin-Configs"] })
        ));

(async () => {


    try {
        let currentVersion = await betterncm.app.getBetterNCMVersion()
        let ncmVersion = document.querySelector(".fstbtn>span").innerText.split(" ")[0]
        let online = await (await fetch("https://gitee.com/microblock/better-ncm-v2-data/raw/master/betterncm/betterncm.json")).json()
        let onlineSuitableVersions = online.versions.filter(v => v.supports.includes(ncmVersion))

        if (onlineSuitableVersions.length === 0) document.querySelector(".BetterNCM-PM-Updatey").appendChild(dom("div", { innerText: decodeURI("BetterNCM%E6%9A%82%E6%9C%AA%E5%AE%98%E6%96%B9%E6%94%AF%E6%8C%81%E8%AF%A5%E7%89%88%E6%9C%AC%EF%BC%8C%E5%8F%AF%E8%83%BD%E4%BC%9A%E5%87%BA%E7%8E%B0Bug") }));

        if (currentVersion != onlineSuitableVersions[0].version) {
            document.querySelector(".BetterNCM-PM-Updatey").appendChild(dom("div",
                {
                    style:
                        { display: "flex", flexDirection: "column", alignItems: "center" }
                },
                dom("div",
                    { innerText: decodeURI("BetterNCM%E6%9C%89%E6%9B%B4%E6%96%B0") + ` (${onlineSuitableVersions[0].version})` }), dom("a", {
                        class: ["u-ibtn5", "u-ibtnsz8"], innerText: decodeURI("%E7%82%B9%E6%AD%A4%E6%9B%B4%E6%96%B0"),
                        onclick: async () => {
                            let ncmpath = await betterncm.app.getNCMPath(), datapath = await betterncm.app.getDataPath(), dllpath = datapath + "\\betterncm.dll";
                            if (await betterncm.fs.exists("./betterncm.dll")) await betterncm.fs.remove("./betterncm.dll")

                            await betterncm.fs.writeFile("./betterncm.dll", await (await fetch(onlineSuitableVersions[0].file)).blob())

                            if (!ncmpath.toLowerCase().includes("system")) {
                                betterncm.app.exec(`cmd /c @echo off & echo BetterNCM Updating... & cd /d C:/ & cd C:/ & cd /d ${ncmpath[0]}:/ & cd "${ncmpath}" & taskkill /f /im cloudmusic.exe>nul & taskkill /f /im cloudmusicn.exe>nul & ping 127.0.0.1>nul & del msimg32.dll & move "${dllpath}" .\\msimg32.dll & start cloudmusic.exe`, true)
                            }
                        }
                    })))
        }

    } catch (e) { }

   
})();
(async ()=>{
    await betterncm.utils.waitForFunction(() => window.loadedPlugins)

    const tools = {
        makeBtn(text, onclick, smaller = false, args = {}) {
            return dom("a", { class: ["u-ibtn5", smaller && "u-ibtnsz8"], innerText: text, onclick, ...args })
        },
        makeCheckbox(args = {}) {
            return dom("input", { type: "checkbox", ...args })
        },
        makeInput(value, args = {}) {
            return dom("input", { value, class: ["u-txt", "sc-flag"], ...args })
        }
    }

    for (let name in loadedPlugins) {
        document.querySelector(".BetterNCM-Plugin-Configs").appendChild(dom("div", { innerText: name, style: { fontSize: "18px", fontWeight: 600 } }))
        document.querySelector(".BetterNCM-Plugin-Configs").appendChild(dom("div", { style: { padding: "7px" } }, loadedPlugins[name].injects[0]._config?.call(loadedPlugins[name], tools)))
    }
})()

})
)";

const auto list_fix_script = R"(

//betterncm.waitForElement("head").then(head=>head.appendChild(dom("style",{innerHTML:`
//.m-plylist-pl2 ul .lst {
//    padding: 0 !important;
//    counter-reset: tlistorder 0 !important;
//}
//`})));
//
//
//betterncm.waitForElement(".lst").then(async ele=>{
//while(1){
//for(let child of document.querySelector(".lst").children){
//    await betterncm.delay(400)
//    child.style.display="block"
//}
//}
//});
//
//console.log("Loaded")

)";


string App::readConfig(const string& key, const string& def) {
	auto configPath = datapath + "/config.json";
	if (!fs::exists(configPath))write_file_text(configPath, "{}");
	auto json = nlohmann::json::parse(read_to_string(configPath));
	if (!(json[key].is_string()))json[key] = def;
	write_file_text(configPath, json.dump());
	return json[key];
}

void App::writeConfig(const string& key, const string& val) {
	auto configPath = datapath + "/config.json";
	if (!fs::exists(configPath))write_file_text(configPath, "{}");
	auto json = nlohmann::json::parse(read_to_string(configPath));
	json[key] = val;
	write_file_text(configPath, json.dump());
}

void exec(string cmd, bool ele, bool showWindow = false) {
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };

	vector<string> result;
	pystring::split(cmd, result, " ");

	vector<string> args;
	for (int x = 1; x < result.size(); x++) {
		args.push_back(result[x]);
	}
	auto file = s2ws(result[0]);
	auto eargs = s2ws(pystring::join(" ", args));
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.lpFile = file.c_str();
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.lpParameters = eargs.c_str();
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = NULL;
	if (ele)
		shExecInfo.lpVerb = L"runas";
	else
		shExecInfo.lpVerb = L"open";



	shExecInfo.lpDirectory = NULL;
	if (showWindow)shExecInfo.nShow = SW_SHOW;
	else shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;

	ShellExecuteEx(&shExecInfo);
}

#define checkApiKey if (!req.has_header("BETTERNCM_API_KEY") || req.get_header_value("BETTERNCM_API_KEY") != apiKey) { res.status = 401; return; }

std::thread* App::create_server(string apiKey) {
	return new std::thread([=] {
		httplib::Server svr;
		this->httpServer = &svr;

	svr.Get("/api/fs/read_dir", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			vector<string> paths;

			if (path[1] == ':') {
				for (const auto& entry : fs::directory_iterator(path))
					paths.push_back(entry.path().string());
			}
			else {
				for (const auto& entry : fs::directory_iterator(datapath + "/" + path))
					paths.push_back(pystring::slice(entry.path().string(), datapath.length() + 1));
			}

			res.set_content(((nlohmann::json)paths).dump(), "application/json");
			});

		svr.Get("/api/fs/read_file_text", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				res.set_content(read_to_string(datapath + "/" + path), "text/plain");
			}
			else {
				res.set_content(read_to_string(path), "text/plain");
			}
			});

		svr.Get("/api/fs/unzip_file", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			auto dist = req.get_param_value("dist");



			if (path[1] != ':') {
				path = datapath + "/" + path;
			}
			
			if (dist[1] != ':') {
				dist = datapath + "/" + dist;
			}

			zip_extract(path.c_str(), dist.c_str(), NULL, NULL);

			res.set_content("ok", "text/plain");
			});

		svr.Get("/api/fs/mkdir", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				fs::create_directories(datapath + "/" + path);
				res.status = 200;
			}
			else {
				fs::create_directories(path);
				res.status = 200;
			}
			});

		svr.Get("/api/fs/exists", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				res.set_content(fs::exists(datapath + "/" + path) ? "true" : "false", "text/plain");
			}
			else {
				res.set_content(fs::exists(path) ? "true" : "false", "text/plain");
			}
			});

		svr.Post("/api/fs/write_file_text", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				write_file_text(datapath + "/" + path, req.body);
				res.status = 200;
			}
			else {
				write_file_text(path, req.body);
				res.status = 200;
			}
			});

		svr.Post("/api/fs/write_file", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				auto file = req.get_file_value("file");
				ofstream ofs(datapath + "/" + path, ios::binary);
				ofs << file.content;

				res.status = 200;
			}
			else {
				auto file = req.get_file_value("file");
				ofstream ofs(path, ios::binary);
				ofs << file.content;

				res.status = 200;
			}
			});

		svr.Get("/api/fs/remove", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			using namespace std;
			namespace fs = std::filesystem;

			auto path = req.get_param_value("path");

			if (path[1] != ':') {
				fs::remove_all(datapath + "/" + path);
				res.status = 200;
			}
			else {
				fs::remove_all(path);
				res.status = 200;
			}
			});



		svr.Post("/api/app/exec", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			auto cmd = req.body;
			exec(cmd, false, req.has_param("_showWindow"));
			res.status = 200;
			});

		svr.Post("/api/app/exec_ele", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			auto cmd = req.body;
			exec(cmd, true, req.has_param("_showWindow"));
			res.status = 200;
			});

		svr.Get("/api/app/datapath", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			res.set_content(datapath, "text/plain");
			});

		svr.Get("/api/app/ncmpath", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			res.set_content(getNCMPath(), "text/plain");
			});

		svr.Get("/api/app/version", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			res.set_content(version, "text/plain");
			});

		svr.Get("/api/app/read_config", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			res.set_content(readConfig(req.get_param_value("key"), req.get_param_value("default")), "text/plain");
			});

		svr.Get("/api/app/write_config", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			writeConfig(req.get_param_value("key"), req.get_param_value("value"));
			res.status = 200;
			});

		svr.Get("/api/app/reload_plugin", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			extractPlugins();
			res.status = 200;
			});

		svr.Get("/api/app/show_console", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			ShowWindow(GetConsoleWindow(), SW_SHOW);
			res.status = 200;
			});

		svr.Get("/api/app/bg_screenshot", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			HWND ncmWin = FindWindow(L"OrpheusBrowserHost", NULL);
			SetWindowDisplayAffinity(ncmWin, WDA_EXCLUDEFROMCAPTURE);
			screenCapturePart(s2ws(datapath + "/screenshot.bmp").c_str());
			res.set_content("http://localhost:3248/local/screenshot.bmp", "text/plain");
			SetWindowDisplayAffinity(ncmWin, WDA_NONE);
			});

		svr.Get("/api/app/get_win_position", [&](const httplib::Request& req, httplib::Response& res) {
			HWND ncmWin = FindWindow(L"OrpheusBrowserHost", NULL);
			int x=0, y=0;
			RECT rect = { NULL };

			int xo = GetSystemMetrics(SM_XVIRTUALSCREEN);
			int yo = GetSystemMetrics(SM_YVIRTUALSCREEN);

			if (GetWindowRect(ncmWin, &rect)) {
				x = rect.left;
				y = rect.top;
			}

			res.set_content((string("{\"x\":")) + to_string(x-xo) + ",\"y\":" + to_string(y-yo) + "}", "application/json");

			});

		svr.Get("/api/app/open_file_dialog", [&](const httplib::Request& req, httplib::Response& res) {
			checkApiKey;
			TCHAR szBuffer[MAX_PATH] = { 0 };
			OPENFILENAME ofn = { 0 };
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			auto filter = s2ws(req.get_param_value("filter"));
			ofn.lpstrFilter = filter.c_str();
			auto initialDir = s2ws(req.get_param_value("initialDir"));
			ofn.lpstrInitialDir = initialDir.c_str();
			ofn.lpstrFile = szBuffer;
			ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
			ofn.nFilterIndex = 0;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
			BOOL bSel = GetOpenFileName(&ofn);

			wstring path = szBuffer;
			res.set_content(ws2s(path), "text/plain");

			});



		svr.set_mount_point("/local", datapath);

		svr.listen("127.0.0.1", 3248);
		});
}

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string random_string(std::string::size_type length)
{
	static auto& chrs = "0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local static std::mt19937 rg{ std::random_device{}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	std::string s;

	s.reserve(length);

	while (length--)
		s += chrs[pick(rg)];

	return s;
}

App::App() {
	extractPlugins();

	auto apiKey = random_string(64);

	server_thread = create_server(apiKey);

	EasyCEFHooks::onKeyEvent = [](auto client, auto browser, auto event) {
		if (event->type == KEYEVENT_KEYUP && 
#if _DEBUG
			event->windows_key_code == 122 //DEBUGģʽ�¸ĳ�F11
#else
			event->windows_key_code == 123
#endif
			) {
			auto cef_browser_host = browser->get_host(browser);
			if (browser->get_host(browser)->has_dev_tools(cef_browser_host))
			{
				browser->get_host(browser)->close_dev_tools(cef_browser_host);
			}
			else
			{
				CefWindowInfo windowInfo {};
				CefBrowserSettings settings {};
				CefPoint point {};
				windowInfo.SetAsPopup(NULL, "EasyCEFInject DevTools");
				cef_browser_host->show_dev_tools(cef_browser_host, &windowInfo, client, &settings, &point);
			}
		}
	};

	EasyCEFHooks::onLoadStart = [=](_cef_browser_t* browser, _cef_frame_t* frame, auto transition_type) {
		if (frame->is_main(frame) && frame->is_valid(frame)) {
			wstring url = frame->get_url(frame)->str;
			EasyCEFHooks::executeJavaScript(frame, "const BETTERNCM_API_KEY='" + apiKey + "';");
			EasyCEFHooks::executeJavaScript(frame, api_script);
			EasyCEFHooks::executeJavaScript(frame, loader_script);
			EasyCEFHooks::executeJavaScript(frame, plugin_manager_script);
			EasyCEFHooks::executeJavaScript(frame, list_fix_script);

			auto loadStartupScripts = [&](string path) {
				if (fs::exists(path))
					for (const auto file : fs::directory_iterator(path)) {
						if (fs::exists(file.path().string() + "/startup_script.js")) {
							EasyCEFHooks::executeJavaScript(frame, read_to_string(file.path().string() + "/startup_script.js"));
						}
					}
			};

			loadStartupScripts(datapath + "/plugins_runtime");
			loadStartupScripts(datapath + "/plugins_dev");
		}
	};

	EasyCEFHooks::onAddCommandLine = [&](string arg) {
		return pystring::index(arg, "disable-gpu") == -1;
	};

	EasyCEFHooks::InstallHooks();
}
App::~App() {
	if (httpServer)
		httpServer->stop(); //��ʵ�ֻ������̰߳�ȫ��
	HANDLE hThread = server_thread->native_handle();
	if (WaitForSingleObject(hThread, 4000) == WAIT_TIMEOUT)
		::TerminateThread(hThread, 0);
	server_thread->detach(); 
	delete server_thread;
	httpServer = nullptr;
	EasyCEFHooks::UninstallHook();

	if (fs::exists(datapath + "/plugins_runtime"))fs::remove_all(datapath + "/plugins_runtime");
}


void App::extractPlugins() {
	error_code ec;
	if (fs::exists(datapath + "/plugins_runtime"))fs::remove_all(datapath + "/plugins_runtime", ec);

	fs::create_directories(datapath + "/plugins_runtime");

	for (auto file : fs::directory_iterator(datapath + "/plugins")) {
		auto path = file.path().string();
		if (pystring::endswith(path, ".plugin")) {
			zip_extract(path.c_str(), (datapath + "/plugins_runtime/tmp").c_str(), NULL, NULL);
			try {
				auto modManifest = nlohmann::json::parse(read_to_string(datapath + "/plugins_runtime/tmp/manifest.json"));
				if (modManifest["manifest_version"] == 1) {
					write_file_text(datapath + "/plugins_runtime/tmp/.plugin.path.meta", pystring::slice(path, datapath.length()));
					fs::rename(datapath + "/plugins_runtime/tmp", datapath + "/plugins_runtime/" + (string)modManifest["name"]);
				}
				else {
					throw new exception("Unsupported manifest version.");
				}
			}
			catch (exception e) {
				write_file_text(datapath + "/log.log", string("\nPlugin Loading Error: ") + (e.what()), true);
				fs::remove_all(datapath + "/plugins_runtime/tmp");
			}

		}
	}
}