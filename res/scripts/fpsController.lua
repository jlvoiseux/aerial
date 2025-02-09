function update(dt)
    local speed = 50.0
    if isKeyPressed("W") then
        moveForward(speed * dt)
    end
    if isKeyPressed("S") then
        moveForward(-speed * dt)
    end
    if isKeyPressed("A") then
        moveRight(-speed * dt)
    end
    if isKeyPressed("D") then
        moveRight(speed * dt)
    end
    if isKeyPressed("SPACE") then
        moveUp(speed * dt)
    end
    if isKeyPressed("LEFT_CTRL") then
        moveUp(-speed * dt)
    end

    local mouseX, mouseY = getMouseDelta()
    rotate(mouseX * dt * 2.0, mouseY * dt * 2.0)
end