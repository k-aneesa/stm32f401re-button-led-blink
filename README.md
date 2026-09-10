

https://github.com/user-attachments/assets/1e97c008-8280-4636-99a9-e4a97197a91e



# stm32f401re-button-led-blink
STM32F401Re Nucleo-64: custom GPIO driver demo-onboard button toggles onboard LED, two external LEDs blink alternately, Bare-metal C, no HAL


# STM32F401RE Nucleo-64 GPIO Driver Assignment

Built against the provided driver: `stm32f401RE.h`, `stm32f401RE_gpio.h`,
`stm32f401RE_gpio.c`. Board: Nucleo-64 (F401RE).



## Pin mapping

| Function            | Pin  | Driver macro                          | Notes                                                                 |
|----------------------|------|----------------------------------------|------------------------------------------------------------------------|
| Onboard LED (LD2)    | PA5  | `GPIOA`, `GPIO_PinNo_5`                | Standard onboard LED on all Nucleo-64 boards                          |
| Onboard button (B1)  | PC13 | `GPIOC`, `GPIO_PinNo_13`               | Active-low. No external pull-up on newer board revisions — internal pull-up enabled via 	`GPIO_PinPuPd = GPIO_NO_PU` |
| External LED 1       | PA6  | `GPIOA`, `GPIO_PinNo_6`                | Free pin on Arduino header CN10 (D12), not used by LD2/B1/ST-LINK VCP |
| External LED 2       | PA7  | `GPIOA`, `GPIO_PinNo_7`                | Free pin on Arduino header CN10 (D11), adjacent to PA6 for easy wiring|

## A note on the driver's `GPIO_NO_PU` macro

In `stm32f401RE_gpio.h`, the pull-up/down macros are:
```c
#define GPIO_NO_PUPD   0   // no pull-up/pull-down
#define GPIO_NO_PU     1   // pull-up
#define GPIO_NO_PD     2   // pull-down
```
Despite the `GPIO_NO_PU` name reading like "no pull-up," its value (`1`) is
what `GPIO_Init()` writes into `PUPDR` as the pull-**up** selection — so
`GPIO_PinPuPd = GPIO_NO_PU` is what actually enables the button's internal
pull-up. This is called out in a comment in `main.c` so it isn't mistaken
for disabling the pull-up.

## Wiring diagram (Task 2)

```
Nucleo D12 (PA6) ----[220-330 ohm resistor]----[LED1 anode | cathode]----+
Nucleo D11 (PA7) ----[220-330 ohm resistor]----[LED2 anode | cathode]----+---- GND (CN6)
```

```
        PA6 ●──/\/\/\──▶|── (LED1) ──┐
                                     ├──● GND
        PA7 ●──/\/\/\──▶|── (LED2) ──┘
```

- Current-limiting resistor (220 ohm–330 ohm typical for a 20 mA LED at
  3.3 V logic) in series with each LED. Resistor position (before or after
  the LED in the loop) does not matter electrically.
- LED anode (longer leg) toward the GPIO pin/resistor side, cathode
  (shorter leg/flat edge) toward GND.
- Both LEDs' cathode branches (after their own resistor) join at one
  common ground node, then a single wire runs from that node to any GND
  pin on the Nucleo board — the GND pin on the power header (CN6) is a
  convenient choice.
- No external wiring needed for Task 1 — LD2 and B1 are already on-board.

## Driver functions used

- `GPIO_PeriClockControl()` — enable AHB1 clock for GPIOA/GPIOC before init.
- `GPIO_Init()` — configure mode, speed, pull, and output type for all four pins.
- `GPIO_WritetoOutputPin()` — drive LD2 and the two external LEDs.
- `GPIO_ReadFromInputPin()` — poll B1.
- `GPIO_ToggleOutputPin()` — flip LD2 on a confirmed button press.

No direct register access is used anywhere in `main.c`; all register-level
work happens inside the driver as provided.

## Behavior

- **Task 1 — Button-controlled LED:** B1 is polled once per main-loop
  iteration. A press is confirmed with a debounce delay, LD2 is toggled
  exactly once, then the code blocks until the button is released (also
  debounced) before returning to the loop. This press-then-release edge
  handling — rather than reacting to the raw "pin is low" level — is what
  prevents flicker or multiple toggles per physical press, including if
  the button is held down.
- **Task 2 — Alternating external LEDs:** PA6 and PA7 are driven 180
  degrees out of phase, 250 ms ON/OFF each, for a 500 ms shared period,
  using only a busy-wait software delay (no timers/SysTick, per the
  assignment requirement).
- Both tasks share `main()`'s `while(1)` loop: each iteration checks the
  button once, then advances the LED-blink pattern by one 500 ms step.

## Observations / known limitations

- `delay_ms()` and `delay_debounce()` use plain loop-count constants
  (`ms * 4000` and `50000` respectively), not a hardware timer, so their
  accuracy depends on the actual SYSCLK and compiler optimization level.
  If the demo blink period visibly drifts from 500 ms, adjust the `4000`
  multiplier in `delay_ms()`.
- `Handle_Button_Toggle()` blocks while B1 is held down, so a long hold
  pauses the LED blink pattern until release. For this assignment's scope
  (single, deliberate presses) that trade-off keeps the toggle logic
  simple and reliable; a fully non-blocking version would sample the
  button on a timer interrupt instead.
- Because the button is only checked once per 500 ms loop iteration (in
  between the two external-LED phases), a very quick tap-and-release can
  land entirely inside the 500 ms window and go undetected. **Press and
  hold B1 for at least half a second** to guarantee the check catches it
  while both tasks are running together. This was confirmed during
  testing: isolating Task 1 alone (commenting out the `ExtLeds_Init()`
  and `Blink_External_Leds_Step()` calls in `main()`) responds to even a
  brief tap, since nothing else competes for loop time.
- The provided `GPIO_Init()` has its alternate-function (`AFR`) block
  commented out — not an issue here since both tasks only use plain
  digital `GPIO_MODE_IN`/`GPIO_MODE_OUT`, no alternate-function pins.
- Internal pull-up on PC13 is required in software (see the `GPIO_NO_PU`
  note above) — newer Nucleo-64 revisions do not populate an external
  pull-up resistor for B1, and omitting this is the most common cause of
  unreliable/flickering button reads on this specific pin.

## Demo video checklist

- [ ] Show the board powered and idle (external LEDs already alternating).
- [ ] Press and hold B1 briefly, show LD2 turning on.
- [ ] Press and hold B1 again, show LD2 turning off — confirm no
      flicker/double-toggle.
- [ ] Hold camera on the external LEDs long enough to show the smooth
      alternating (out-of-phase) pattern for several cycles.
