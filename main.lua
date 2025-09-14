local MCMLoaded, MCM = pcall(require, "scripts.modconfig")



if MCMLoaded and MCM then
    local mod = RegisterMod("ChineseRepp",1)

    local json = require("json")

    local cn = MCM.i18n == "Chinese"

    local cfg = {
        fix_input = true,
        revive = true,
        emoji = true
    }

    local data_str = Isaac.LoadModData(mod)
    if data_str and data_str ~= "" then
        cfg = json.decode(data_str)
    end

    local function save()
        cfg.hint = "cncf" .. (cfg.fix_input and "1" or "0") .. 
            "cncr" .. (cfg.revive and "1" or "0") ..
            "cnce" .. (cfg.emoji and "1" or "0")
        Isaac.SaveModData(mod, json.encode(cfg))
    end

    MCM.AddText(cn and "官中+" or "ReppCN", cn and "以下内容将在重启后生效" or "The following setthings need restart game.")

    MCM.AddSetting(cn and "官中+" or "ReppCN", {
        Type = ModConfigMenu.OptionType.BOOLEAN,
        CurrentSetting = function() return cfg.fix_input end,
        Display = cn    and function() return "输入法修复:" .. (cfg.fix_input and "开" or "关") end
                        or  function() return "fix input:" .. (cfg.fix_input and "on" or "off") end,
        OnChange = function(b) cfg.fix_input = b save() end,
        Info =cn    and {"修复中文输入法。"}
                    or  {"Chinese input fix."}
    })

    MCM.AddSetting(cn and "官中+" or "ReppCN", {
        Type = ModConfigMenu.OptionType.BOOLEAN,
        CurrentSetting = function() return cfg.revive end,
        Display = cn    and function() return "复活机贴图:" .. (cfg.revive and "开" or "关") end
                        or  function() return "revive:" .. (cfg.revive and "on" or "off") end,
        OnChange = function(b) cfg.revive = b save() end,
        Info =cn    and {"替换复活机。"}
                    or  {"revive mach rep."}
    })    
    
    MCM.AddSetting(cn and "官中+" or "ReppCN", {
        Type = ModConfigMenu.OptionType.BOOLEAN,
        CurrentSetting = function() return cfg.emoji end,
        Display = cn    and function() return "表情:" .. (cfg.emoji and "开" or "关") end
                        or  function() return "emoji:" .. (cfg.emoji and "on" or "off") end,
        OnChange = function(b) cfg.emoji = b save() end,
        Info =cn    and {"替换表情。"}
                    or  {"emoji replace."}
    })

end