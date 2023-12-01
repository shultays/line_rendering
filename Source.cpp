
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

struct Vec {
    float x, y;
    Vec operator+(const Vec& other) const {
        return { x + other.x, y + other.y };
    }
    Vec operator-(const Vec& other) const {
        return { x - other.x, y - other.y };
    }
    Vec operator*(float t) const {
        return { x * t, y * t };
    }
    Vec operator/(float t) const {
        return { x / t, y / t };
    }
    Vec operator-() const {
        return { -x, -y };
    }
    Vec rot() const {
        return { -y, +x };
    }
    float dot(const Vec& other) const {
        return x * other.x + y * other.y;
    }
    Vec normalized() const {
        const float l = length();
        return { x / l, y / l };
    }
    float length() const {
        return sqrtf(x * x + y * y);
    }
};

struct VertexData {
    Vec v;
    float l;
};

struct Mesh {
    std::vector<VertexData> vertices;
    std::vector<unsigned> indices;
};

Mesh createLine(const std::vector<Vec>& points, float w) {
    std::vector<Vec> vertices0;
    std::vector<Vec> vertices1;
    const unsigned size = (unsigned)points.size();
    vertices0.reserve(points.size());
    vertices1.reserve(points.size());
    for (unsigned i = 0; i < size; ++i) {
        const Vec prev = points[ ( i + size - 1) % size ];
        const Vec next = points[ ( i + 1 ) % size ];
        const Vec cur = points[ i ];

        const Vec dir0 = ( cur - prev ).normalized();
        const Vec dir1 = ( next - cur ).normalized();

        const Vec r0 = dir0.rot();
        const Vec r1 = dir1.rot();

        const float cross = dir0.x * dir1.y - dir0.y * dir1.x;

        auto fun = [&](const Vec p0, const Vec p1) {
            const float t = ( ( p1.x - p0.x ) * dir1.y - ( p1.y - p0.y ) * dir1.x ) / cross;
            return p0 + dir0 * t;
        };

        vertices0.push_back(fun(cur + r0 * w, cur + r1 * w));
        vertices1.push_back(fun(cur - r0 * w, cur - r1 * w));
    }

    Mesh mesh;
    float l = 0.0f;
    for (unsigned i = 0; i < size; ++i) {
        unsigned ni = ( i + 1 ) % size;

        float lCur = ( points[ ni ] - points[ i ] ).length();
        const Vec dir = (points[ ni ] - points[ i ]) / lCur;

        mesh.vertices.push_back({ vertices0[ i ], l + ( vertices0[ i ] - points[ i ] ).dot(dir) });
        mesh.vertices.push_back({ vertices1[ i ], l + ( vertices1[ i ] - points[ i ] ).dot(dir) });
        mesh.vertices.push_back({ vertices0[ ni ], l + ( vertices0[ ni ] - points[ i ] ).dot(dir) });
        mesh.vertices.push_back({ vertices1[ ni ], l + ( vertices1[ ni ] - points[ i ] ).dot(dir) });

        l += lCur;
        unsigned t = i * 4;

        mesh.indices.push_back(t);
        mesh.indices.push_back(t + 1);
        mesh.indices.push_back(t + 2);

        mesh.indices.push_back(t + 1);
        mesh.indices.push_back(t + 3);
        mesh.indices.push_back(t + 2);
    }

    return mesh;
}

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::ifstream vsFile; 
    vsFile.open("vs.txt");
    std::string vs(( std::istreambuf_iterator<char>(vsFile) ),
                   std::istreambuf_iterator<char>());
    const char* c = vs.c_str();
    glShaderSource(vertexShader, 1, &c, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[ 512 ];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::ifstream psFile;
    psFile.open("ps.txt");
    std::string ps(( std::istreambuf_iterator<char>(psFile) ),
                   std::istreambuf_iterator<char>());
    c = ps.c_str();
    glShaderSource(fragmentShader, 1, &c, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<Vec> points{ {100.0f, 100.0f}, {400.0f, 150.0f}, {400.0f, 350.0f}, {300.0f, 200.f}, {120.0f, 150.0f} };
    Mesh m = createLine(points, 3);

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m.vertices[0]) * m.vertices.size(), &m.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m.indices[ 0 ]) * m.indices.size(), &m.indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, (unsigned)m.indices.size(), GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

