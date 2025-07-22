return {
  "rcarriga/nvim-notify",
  config = function()
    require("notify").setup({
      background_colour = "#000000", -- Example: Black background for notifications
      timeout = 3000, -- Notifications disappear after 3 seconds
      max_height = function() return math.floor(vim.opt.rows:get() * 0.75) end,
      max_width = function() return math.floor(vim.opt.columns:get() * 0.75) end,
      -- You can also set a level filter here globally
      -- levels = "error", -- Only show errors globally
      -- Alternatively, you can disable rendering certain types of messages:
      -- render = "compact", -- or "minimal" etc.
    })
  end
}
