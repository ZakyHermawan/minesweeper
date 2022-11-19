#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include <vector>
#include <random>
#include <queue>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int right_click_pressed;
int x_right_click_pressed;
int y_right_click_pressed;
int last_x, last_y;

int number_of_mines;
int grid_width;
int grid_height;
int window_width;
int window_height;


static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  auto& io = ImGui::GetIO();
  io.AddMouseButtonEvent(button, action);
  if (button == 1 && action == 1) {
    right_click_pressed = 1;
  }
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  auto& io = ImGui::GetIO();
  io.AddMousePosEvent(xpos, ypos);
  last_x = xpos / (window_width / grid_width);
  last_y = ypos / (window_height / grid_height);
}

std::vector<std::pair<int, int>> reservoir_sampling(std::vector<std::vector<int>>& v, int sample_size) {
  std::vector<std::pair<int, int>> result;
  std::random_device rd;
  std::default_random_engine generator(rd());

  std::uniform_int_distribution<int> index_dist(0, sample_size - 1);
  std::uniform_real_distribution<double> zero_one_dist(0.0, 1.0);
  double W = std::pow(zero_one_dist(generator), 1.0 / sample_size);
  int next_idx = std::log(zero_one_dist(generator)) / std::log(1 - W);

  for (int i = 0; i < v.size(); ++i) {
    for (int j = 0; j < v[0].size(); ++j) {
      if (result.size() < sample_size) {
        result.push_back(std::make_pair(i, j));
      }
      else if (--next_idx <= 0) {
        result[index_dist(generator)] = std::make_pair(i, j);
        W *= std::exp(std::log(zero_one_dist(generator)) / sample_size);
        next_idx = std::log(zero_one_dist(generator)) / std::log(1 - W);
      }
    }
  }
  return result;
}

int main(int, char**)
{
  srand(time(0));
  number_of_mines = 5;
  grid_width = 5;
  grid_height = 5;
  window_width = 1280;
  window_height = 720;

  std::vector<std::vector<int>> grid(grid_height, std::vector<int>(grid_width));
  std::vector<std::pair<int, int>> bombs_coordinate = reservoir_sampling(grid, number_of_mines);
  for (auto& pr : bombs_coordinate) {
    grid[pr.first][pr.second] = -1;
  }
  
  for (int i = 0; i < grid_height; ++i) {
    for (int j = 0; j < grid_width; ++j) {
      if (grid[i][j] == -1) continue;
      if (i > 0 && grid[i - 1][j] == -1) {
        ++grid[i][j];
      }
      if (i > 0 && j < grid_width - 1 && grid[i - 1][j + 1] == -1) {
        ++grid[i][j];
      }
      if (j < grid_width - 1 && grid[i][j + 1] == -1) {
        ++grid[i][j];
      }
      if (i < grid_height - 1 && j < grid_width - 1 && grid[i + 1][j + 1] == -1) {
        ++grid[i][j];
      }
      if (i < grid_height - 1 && grid[i + 1][j] == -1) {
        ++grid[i][j];
      }
      if (i < grid_height - 1 && j > 0 && grid[i + 1][j - 1] == -1) {
        ++grid[i][j];
      }
      if (j > 0 && grid[i][j - 1] == -1) {
        ++grid[i][j];
      }
      if (i > 0 && j > 0 && grid[i - 1][j - 1] == -1) {
        ++grid[i][j];
      }
    }
  }
  
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Minesweeper", NULL, NULL);
  if (window == NULL)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  
  // Our state
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  std::vector<int> pressed_buttons(grid_height * grid_width);
  bool is_game_over = false;
  bool start_new_game = false;
  right_click_pressed = 0;

  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if(!is_game_over) {
      ImGui::SetWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::Begin("Window bikinan sendiri", NULL, ImGuiWindowFlags_NoTitleBar);
      ImGui::Columns(grid_width, NULL);
      ImGui::Separator();
      char name[16];
      bool is_finished = true;
      for (int i = 0; i < grid_height * grid_width; ++i) {
        if (i > 0 && i % grid_width == 0) {
          ImGui::Separator();
        }
        if (pressed_buttons[i] == 0) {
          is_finished = false;
          char buff[32];
          if (right_click_pressed && i == last_y * grid_width + last_x) {
            sprintf_s(buff, "Flag###%d", i);
            pressed_buttons[i] = 2;
            ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height));
            continue;
          }
          else {
            sprintf_s(buff, "###%d", i);
          }

          if (ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height))) {
            pressed_buttons[i] = 1;
            int grid_value = grid[i / grid_height][i % grid_width];
            if (grid_value == -1) {
              is_game_over = true;
              printf("Game over di posisi (%d, %d) dengan value %d\n", i% grid_width, i/ grid_height, grid_value);
            }
          }
        }
        else if (pressed_buttons[i] == 2) {
          char buff[32];
          if (right_click_pressed && i == last_y * grid_width + last_x) {
            sprintf_s(buff, "Flag###%d", i);
            ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height));
            pressed_buttons[i] = 0;
          }
          else {
            sprintf_s(buff, "Flag###%d", i);
            ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height));
          }
        }
        else {
          bool is_bomb = (grid[i / grid_height][i % grid_width] == -1) ? true : false;
          if (is_bomb) {
            sprintf_s(name, "%d", i);
            char buff[10];
            sprintf_s(buff, "X###%d", i);
            ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height));
          }
          else {
            sprintf_s(name, "%d", i);
            char buff[10];
            sprintf_s(buff, "%d###%d", grid[i / grid_height][i % grid_width], i);
            ImGui::Button(buff, ImVec2(io.DisplaySize.x / grid_width, io.DisplaySize.y / grid_height));
          }
        }
        ImGui::NextColumn();
      }
      if (is_finished) {
        int count_flag = 0;
        for (int i = 0; i < grid_height * grid_width; ++i) {
          if (pressed_buttons[i] == 2) ++count_flag;
        }
        if (count_flag != number_of_mines) {
          is_finished = false;
        }
      }
      if (is_finished) {
        bool is_win = true;
        for (int i = 0; i < grid_height; ++i) {
          for (int j = 0; j < grid_width; ++j) {
            if (grid[i][j] == -1 && pressed_buttons[i * grid_width + j] != 2) {
              printf("fail\n");
              i = grid_height;
              is_win = false;
              continue;
            }
          }
        }
        if (is_win) {
          printf("win\n");
        }
      }
      ImGui::Columns(1);
      ImGui::Separator();
      
      ImGui::End();
      right_click_pressed = 0;
    }
    else {
      ImGui::SetNextWindowSize(ImVec2(700, 500));
      ImGui::Begin("Window bikinan sendiri", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
      ImGui::Text("Game Over");
      ImGui::End();
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
