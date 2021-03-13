#include <SFML/Graphics.hpp>
#include <string>


#define WIDTH 1024

int main()
{
    // create the window
    sf::RenderWindow window(sf::VideoMode(WIDTH, WIDTH), "My window");

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        
        // Draw the Grid
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                // init rectangle
                sf::RectangleShape tempRect(sf::Vector2f(WIDTH/8, WIDTH/8));
                
                // Make checkerboard pattern
                if ((x + y) % 2 == 0) {
                    tempRect.setFillColor(sf::Color(150, 50, 250));
                }
                else {
                    tempRect.setFillColor(sf::Color(150, 50, 200));
                }
                
                // move to proper location and draw
                tempRect.move(x * WIDTH/8, y * WIDTH/8);
                window.draw(tempRect);
            }
        }
        
        // Draw the pieces
        
        
        
        // end the current frame
        window.display();
    }

    return EXIT_SUCCESS;
}
