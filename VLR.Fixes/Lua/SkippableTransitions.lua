-- If true, skip the next wait() or onLerp() call
skipActive = false

-- If > 0, the next N calls to wait() or onLerp() can be skipped
skipRemaining = 0

-- Whether the last wait() or onLerp() was skipped and the skip chain has ended
skipWasActive = false

shouldSkip = function()
  if skipActive then
    return true
  end
  if skipRemaining > 0 and (INPUT:trigger(PAD.A) or INPUT.touch) then
    skipActive = true
    return true
  end
  return false
end  

decrementSkipRemaining = function()
  skipWasActive = false
  if skipRemaining > 0 then
    skipRemaining = skipRemaining - 1
    if skipRemaining == 0 then
      skipActive = false
      skipWasActive = true
    end
  end
end

----------------
-- thread.lua --
----------------

thread.wait = function(time)
  time = time / thread.SKIP_SPEED
  local t = TIMER:getTime()
  while TIMER:getTime() - t < 1000 * (time) / FPS do
    if shouldSkip() then
      break
    end
    (coroutine.yield)("")
  end
  decrementSkipRemaining();
  (coroutine.yield)("")
end

--------------
-- base.lua --
--------------

_G.onLerp = function(self, i, move)
  local finish = false
  if move.finish == false then
    move.time = move.time + SYSTEM:getDeltaTime()
    if move.span <= move.time or skipActive then
      move.time = move.span
      move.finish = true
    end
    if move.sync ~= false and shouldSkip() then
      move.time = move.span
      move.finish = true
    end
  else
    if move.sync ~= false then
      (move.thread):resume()
    end
    (self.move)[move.id] = nil
    if size(self.move) == 0 then
      self.move = nil
      (move.self).onUpdate = nil
    end
    finish = true
    if move.sync ~= false then
      decrementSkipRemaining()
    end
  end
  local t = nil
  if move.time_func == nil then
    t = move.time / move.span
  else
    t = ((move.time_func).func)(move.time / move.span)
  end
  local p = lerp_(move.from, move.to, t);
  (move.onSet)(p, finish)
end

-------------
-- map.lua --
-------------

local point = {
  {"A", vector2(415, 268)},
  {"B", vector2(472, 268)},
  {"C", vector2(530, 268)},
  {"D", vector2(591, 268)},
  {"E", vector2(591, 253)},
  {"test", vector2(146, 57)},
  {"warehouse_a", vector2(414, 338)},
  {"warehouse_b", vector2(321, 419)},
  {"warehouse_c", vector2(472, 338)},
  {"elevatorA", vector2(496, 81)},
  {"elevatorB", vector2(472, 81)},
  {"elevatorC", vector2(473, 104)},
  {"elevatorD", vector2(449, 81)},
  {"lounge", vector2(624, 139.5)},
  {"dispensary_A", vector2(567, 267.5)},
  {"dispensary_B", vector2(591, 256)},
  {"cabin", vector2(594.5, 337.5)},
  {"gaulem", vector2(519, 151)},
  {"storehouse", vector2(204, 163)},
  {"data_room", vector2(799, 103)},
  {"recreation", vector2(600, 244)},
  {"treat", vector2(717, 81)},
  {"treat_B", vector2(728, 105)},
  {"control", vector2(425.5, 221)},
  {"monitor", vector2(659, 432)},
  {"decompression_A", vector2(603, 337)},
  {"decompression_B", vector2(634, 290.5)},
  {"biotope", vector2(748, 291)},
  {"labo_A", vector2(226, 337.5)},
  {"labo_B", vector2(146, 256)},
  {"chief", vector2(213, 419)},
  {"final", vector2(99, 409)},
  {"A1", vector2(624, 81)},
  {"A2", vector2(671, 81)},
  {"A3", vector2(624, 198)},
  {"A4", vector2(671, 198)},
  {"A5", vector2(414, 268)},
  {"A6", vector2(472, 268)},
  {"A7", vector2(531, 268)},
  {"A8", vector2(671, 256)},
  {"A9", vector2(531, 338)},
  {"A10", vector2(671, 338)},
  {"A11", vector2(414, 232)},
  {"A12", vector2(523, 232)},
  {"A13", vector2(555, 198)},
  {"B1", vector2(146, 57)},
  {"B2", vector2(262, 57)},
  {"B3", vector2(426, 81)},
  {"B4", vector2(519, 81)},
  {"B5", vector2(671, 81)},
  {"B6", vector2(671, 34)},
  {"B7", vector2(799, 34)},
  {"B8", vector2(146, 163)},
  {"B9", vector2(99, 221)},
  {"B10", vector2(146, 221)},
  {"B11", vector2(204, 221)},
  {"B12", vector2(262, 221)},
  {"B13", vector2(473, 198)},
  {"B14", vector2(531, 198)},
  {"B15", vector2(671, 151)},
  {"B16", vector2(671, 198)},
  {"B17", vector2(717, 198)},
  {"B18", vector2(799, 198)},
  {"B19", vector2(99, 337)},
  {"B20", vector2(262, 337)},
  {"B21", vector2(414, 268)},
  {"B22", vector2(473, 268)},
  {"B23", vector2(531, 268)},
  {"B24", vector2(531, 338)},
  {"B25", vector2(671, 244)},
  {"B26", vector2(671, 290)},
  {"B27", vector2(169, 419)},
  {"B28", vector2(262, 419)},
  {"B29", vector2(659, 384)},
  {"B30", vector2(99, 478)},
  {"B31", vector2(169, 478)},
  {"B32", vector2(262, 478)},
  {"B33", vector2(321, 478)},
  {"B34", vector2(379, 478)},
  {"B35", vector2(531, 478)},
  {"B36", vector2(659, 478)},
  {"B37", vector2(352, 57)},
  {"B38", vector2(399, 81)},
  {"B39", vector2(836, 198)},
  {"B40", vector2(846, 206)},
  {"B41", vector2(846, 376)},
  {"B42", vector2(838, 384)},
  {"B43", vector2(531, 384)},
  {"B44", vector2(473, 220)},
  {"B45", vector2(169, 359)},
  {"B46", vector2(149, 337)},
  {"B47", vector2(239, 57)},
  {"B48", vector2(123, 221)}
}

local TIME_H = function(span_)
  return {
    span = span_,
    func = function(t)
      return getSplineValue(t, {
        {0, 0},
        {0.9, 1.1},
        {0.9, 1.1},
        {1, 1}
      })
    end
  }
end

local getLinearValue = function(t, nums)
  local h = nil
  for i = 1, (table.maxn)(nums) - 1 do
    if (nums[i])[1] <= t and t <= (nums[i + 1])[1] then
      h = i
      break
    end
  end
  do
    if h == nil then
      return (nums[(table.maxn)(nums)])[2]
    end
    local p1 = nums[h + 0]
    local p2 = nums[h + 1]
    return lerp_(p1[2], p2[2], (t - p1[1]) / (p2[1] - p1[1]))
  end
end

map.create = function(type, name, zoom)
  room_name("")
  local bg = ULayer2(ULCD)
  bg.bg = Sprite9();
  (bg.bg):addTo(bg);
  (bg.bg):load("pic/map/bg.etc");
  (bg.bg).depth = 10
  bg.map = Sprite9();
  (bg.map):addTo(bg.bg);
  (bg.map).depth = 7;
  bg.title = Sprite9();
  (bg.title).depth = 8
  (bg.title):addTo(bg.bg)
  if type == MAP_A then
    (bg.map):load("pic/map/a_map.etc");
    (bg.title):load("pic/map/a_floor.etc")
  else
    (bg.map):load("pic/map/b_map.etc");
    (bg.title):load("pic/map/b_floor.etc")
  end
  (bg.map).add = 1;
  (bg.map).noise = Noise();
  (bg.map).onUpdate = function(self)
    self.alpha = ((self.noise):get1D(TIMER:getTime() * 0.002, 1) + 1) / 2 * 204 + 50
  end
  (bg.map).depth = 3
  bg.cursor = Sprite9();
  (bg.cursor):addTo(bg.bg);
  (bg.cursor):load("pic/map/point.etc");
  (bg.cursor).depth = 3
  (bg.cursor).time = 0
  (bg.cursor).add = 1
  bg.tx = 0
  bg.ty = 0
  bg.go = false
  bg.onSet = function(self, p)
    local ms = 0.446
    local x = (-bg.tx + 200) * 3
    local y = (-bg.ty + 120) * 3
    local s = lerp_(ms, 1, p)
    local x1 = x * p
    local y1 = y * p;
    (bg.bg):scale2(s * 1.4, 600 / LCD_SCALE, 300 / LCD_SCALE, x1 / LCD_SCALE, y1 / LCD_SCALE)
    local x = (-bg.tx + 200) * 3
    local y = (-bg.ty + 120) * 3
    local s = lerp_(ms, 1, p)
    local x1 = x * p
    local y1 = y * p;
    (bg.map):scale2(s, 0, 0, x1 / LCD_SCALE, y1 / LCD_SCALE)
    local x1 = bg.tx * 3 * s + x1
    local y1 = bg.ty * 3 * s + y1;
    (bg.cursor):scale2(s, 48 / LCD_SCALE, 48 / LCD_SCALE, x1 / LCD_SCALE, y1 / LCD_SCALE)
  end
  bg.pp = 0
  bg.time8 = 0
  bg.route = {}
  bg.sum = 0
  for i,p in pairs(name) do
    local pos = nil
    for _,v in pairs(point) do
      if v[1] == p then
        pos = v[2]
      end
    end
    if pos == nil then
      lprintf((string.format)("map:no place %s\n", p))
    end
    if i ~= 1 then
      local p0 = ((bg.route)[i - 1])[2]
      local p1 = pos
      bg.sum = bg.sum + (math.sqrt)((p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y))
    end
    do
      do
        (bg.route)[i] = {bg.sum, pos}
      end
    end
  end
  bg.tx = (((bg.route)[1])[2]).x
  bg.ty = (((bg.route)[1])[2]).y
  bg.num = -1
  bg.blink = function(self, b)
    bg.blink_ = b
  end

  bg.canceled = false
  (bg.cursor).onUpdate = function(self)
    if SHUTTER.exist ~= true then
      local speed = 1
      if bg.sum < bg.time8 then
        speed = 1.5
      end
      self.time = self.time + SYSTEM:getDeltaTime() * speed * 14
      local num = (math.modf)(self.time / 3.141592 / 2)
      if bg.blink_ ~= false then
        if num ~= bg.num then
          if bg.sum < bg.time8 or (table.maxn)(bg.route) == 1 then
            if not bg.canceled then
              SE:play("SE_SYS_MAP_LIGHT_STOP")
            end
          else
            SE:play("SE_SYS_MAP_LIGHT_MOVE")
          end
          bg.num = num
        end
        self.alpha = ((math.sin)(self.time) + 1) / 2 * 120 + 120
      else
        self.alpha = 240
      end
      if bg.go == true then
        bg.time8 = bg.time8 + SYSTEM:getDeltaTime() * 80 * GAME.MOTION_SPEED
        local v = getLinearValue(bg.time8, bg.route)
        bg.tx = v.x
        bg.ty = v.y
      end
      do
        bg:onSet(bg.pp)
      end
    end
  end

  bg.proc = function(self)
    bg:onSet(0)
    while SHUTTER.exist == true do
      (thread.yield)()
    end
    if zoom == nil then
      skipRemaining = 4
      wait(60)
      bg.go = true
      wait(30)
      local onSet2 = function(p)
        bg.pp = p
      end

      if zoom ~= false then
        move(bg, 0, 1, TIME_H(75), onSet2, false)
      end
      while bg.time8 < bg.sum do
        if shouldSkip() then
          bg.time8 = bg.sum
          break
        end
        (thread.yield)()
      end
      bg.time8 = bg.sum
      wait(15)
      
      if skipActive then
        bg.canceled = true
      end
    end
    do
      (bg.tt):resume()
    end
  end

  bg.go = function(self)
    bg.tt = (thread.get)()
    bg.t = (thread.create)(bg.proc, nil, bg.tt);
    (thread.suspend)()
  end

  bg.last_frame = function(self)
    if zoom == nil then
      bg.pp = 1
      bg.go = true
      bg.time8 = bg.sum
    end
  end

  bg.stop = function(self)
    self:blink(false)
  end

  bg.kill = function(self)
    ROOM_NAME.visible = bg.room_name;
    (bg.t):terminate()
    bg.t = nil
    bg.tt = nil
  end

  bg.room_name = ROOM_NAME.visible
  ROOM_NAME:hide()
  return bg
end

--------------------
-- room_title.lua --
--------------------

room_title.proc = function(room)
  PIC:fill(BLACK)
  local id0 = PIC:print(0, 0, room);
  (PIC:text(id0)):setFont(FONT.ROOM_NAME);
  (PIC:text(id0)).align = Align.H_CENTER + Align.V_CENTER;
  skipRemaining = 2
  (PIC:text(id0)):show(WAIT_SLOW)
  wait(WAIT_SLOW * 2);
  if skipWasActive then
    (PIC:text(id0)):hide(WAIT_FAST)
  else
    (PIC:text(id0)):hide(WAIT_SLOW)
  end
end

---------------
-- start.lua --
---------------

start.logo = function(tf)
  if REBOOT == 0 and tf then
    if ENABLE_LOGO_MOVIE == false then
      local logo_func = function()
        local layer = function()
          local obj = Layer()
          obj.fade = function(self, time)
            local onSet = function(p)
              obj.alpha = p
            end
            move(obj, 0, 255, time, onSet, false)
          end
          return obj
        end

        FADE_U:hide()
        FADE_L:hide()
        do
          if PUBLISHER_LOGO_IMAGE ~= "" then
            local logo_layer = layer()
            pub_logo = (ui.sprite)(logo_layer, 0, 0, PUBLISHER_LOGO_IMAGE)
            logo_layer:addTo(ULCD)
            logo_layer:show()
            PIC:hide()
            logo_layer:fade(60)
            skipRemaining = 1
            wait(180)
            PIC:fill(BLACK, 60)
            logo_layer:hide()
            cleanup(logo_layer)
            logo_layer = nil
          end
          if DEVELOPER_LOGO_IMAGE ~= "" then
            logo_layer = layer()
            dev_logo = (ui.sprite)(logo_layer, 0, 0, DEVELOPER_LOGO_IMAGE)
            logo_layer:addTo(ULCD)
            logo_layer:show()
            PIC:hide()
            logo_layer:fade(60)
            skipRemaining = 1
            wait(180)
            PIC:fill(BLACK, 60)
            logo_layer:hide()
            cleanup(logo_layer)
            logo_layer = nil
          end
          gc()
        end
      end
      wait(30)
      logo_func()
    else
      do
        do
          local m = PIC:movie("chun_logo_m0", 1, false);
          (thread.yield)();
          (thread.yield)()
          while m:is_playing() == true and INPUT:trigger(PAD.A) ~= true do
            if INPUT.touch == true then
              break
            end;
            (thread.yield)()
          end
          PIC:hide()
          wait(30)
        end
      end
    end
  end
end
