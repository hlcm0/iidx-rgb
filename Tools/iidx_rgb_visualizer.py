import pygame
import mmap
import time
import numpy as np

# Init pygame
pygame.init()

# LED count configuration
LED_COUNTS = [19, 19, 45, 45, 21, 54, 11, 11, 54, 17, 17, 68, 68, 61, 68, 68, 61]
LED_OFFSETS = [0, 19, 38, 83, 128, 149, 203, 214, 225, 279, 296, 313, 381, 449, 510, 578, 646]
LED_NAMES = ['STAGE L', 'STAGE R', 'CAB L', 'CAB R', 'CTRL PNL', 'CEIL L', 
            'TITLE L', 'TITLE R', 'CEIL R', 'TOUCH L', 'TOUCH R', 
            'SIDE L IN', 'SIDE L OUT', 'SIDE L', 'SIDE R OUT', 'SIDE R IN', 'SIDE R']

# Window settings
LED_SIZE = 10  # Each LED is displayed as 10x10 pixels
STRIP_GAP = 2  # Gap between strips
TEXT_WIDTH = 20  # Text width
WINDOW_WIDTH = (LED_SIZE + STRIP_GAP + TEXT_WIDTH) * len(LED_COUNTS)
WINDOW_HEIGHT = LED_SIZE * max(LED_COUNTS)  # Use longest strip length
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("IIDX LED Visualizer")

font = pygame.font.SysFont('Arial', 12)

# Open shared memory
try:
    shm = mmap.mmap(-1, 2121, "iidxrgb")
except Exception as e:
    print(f"Failed to open shared memory: {e}")
    exit(1)

clock = pygame.time.Clock()
running = True

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
    
    # Clear screen
    screen.fill((0, 0, 0))
    
    # Read shared memory data and update display
    shm.seek(0)
    data = shm.read()
    
    # Draw each strip
    for strip_idx in range(len(LED_COUNTS)):
        offset = LED_OFFSETS[strip_idx] * 3
        count = LED_COUNTS[strip_idx]
        x = strip_idx * (LED_SIZE + STRIP_GAP + TEXT_WIDTH)
        
        # Draw all LEDs of this strip
        for led_idx in range(count):
            r = data[offset + led_idx * 3]
            g = data[offset + led_idx * 3 + 1]
            b = data[offset + led_idx * 3 + 2]
            y = led_idx * LED_SIZE
            
            # Draw LED
            pygame.draw.rect(screen, (r, g, b), 
                           (x, y, LED_SIZE, LED_SIZE))
        
        # Draw text
        text = LED_NAMES[strip_idx]
        y_text = 5
        for char in text:
            if char != ' ':
                text_surface = font.render(char, True, (255, 255, 255))
                text_rect = text_surface.get_rect()
                text_rect.centerx = x + LED_SIZE + TEXT_WIDTH//2
                text_rect.top = y_text
                screen.blit(text_surface, text_rect)
                y_text += text_rect.height + 1
    
    pygame.display.flip()
    clock.tick(120)  # Limit to 120 frames per second

pygame.quit() 