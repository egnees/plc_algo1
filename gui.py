import os
import random
import time
import tkinter as tk
from tkinter import ttk
import tkinter.filedialog
from tkinter import messagebox
import placer
from PIL import Image

import matplotlib.pyplot as plt

from scipy.spatial import ConvexHull

# Placer GUI
WINDOW_TITLE = 'Placer GUI'
DEFAULT_WINDOW_WIDTH = 1280
DEFAULT_WINDOW_HEIGHT = 720
DEFAULT_GEOM = str(DEFAULT_WINDOW_WIDTH) + 'x' + str(DEFAULT_WINDOW_HEIGHT)
DEFAULT_NOTEBOOK_HEIGHT = 50
DEFAULT_NOTEBOOK_BG = 'white'
DEFAULT_NOTEBOOK_OUTLINE_WIDTH = 0
DEFAULT_NOTEBOOK_OUTLINE_COLOR = 'black'

DEFAULT_ALIGN_X = 10
DEFAULT_ALIGN_Y = 10

DEFAULT_ALIGN_PIN_X = 5
DEFAULT_ALIGN_PIN_Y = 5

# Devices
DEFAULT_DEVICE_HALF_WIDTH = 25
DEFAULT_DEVICE_HALF_HEIGHT = 25
DEFAULT_DEVICE_OUTLINE_WIDTH = 2
DEFAULT_DEVICE_FILL_COLOR = '#FCE205'
DEFAULT_DEVICE_OUTLINE_COLOR = 'black'
DEFAULT_DEVICE_SELECTED_FILL_COLOR = DEFAULT_DEVICE_FILL_COLOR
DEFAULT_DEVICE_SELECTED_OUTLINE_COLOR = 'blue'
DEFAULT_DEVICE_HOVER_FILL_COLOR = DEFAULT_DEVICE_FILL_COLOR
DEFAULT_DEVICE_HOVER_OUTLINE_COLOR = DEFAULT_DEVICE_SELECTED_OUTLINE_COLOR

# Pins
DEFAULT_PIN_HALF_WIDTH = 5
DEFAULT_PIN_HALF_HEIGHT = 5
DEFAULT_PIN_OUTLINE_WIDTH = 2
DEFAULT_PIN_FILL_COLOR = '#FF3232'
DEFAULT_PIN_OUTLINE_COLOR = 'black'
DEFAULT_PIN_SELECTED_FILL_COLOR = DEFAULT_PIN_FILL_COLOR
DEFAULT_PIN_SELECTED_OUTLINE_COLOR = DEFAULT_DEVICE_SELECTED_OUTLINE_COLOR
DEFAULT_PIN_HOVER_FILL_COLOR = DEFAULT_PIN_FILL_COLOR
DEFAULT_PIN_HOVER_OUTLINE_COLOR = DEFAULT_PIN_SELECTED_OUTLINE_COLOR

# Nets
DEFAULT_NET_OUTLINE_WIDTH = 2
# DEFAULT_NET_COLORS = ['#FF00FF', '#228B22', '#EE82EE',
#                       '#800080', '#FF1493', '#FF7F50',
#                       '#DC143C', '#DAA520', '#800000']
DEFAULT_NET_COLORS = ['#FF00FF', '#3CB371', '#DC143C',
                      '#228B22', '#FF1493', '#DAA520',
                      '#EE82EE', '#FF7F50', '#800000']

DEFAULT_NET_SELECTED_COLOR = 'red'
DEFAULT_NET_SELECTED_OUTLINE_WIDTH = 3
DEFAULT_NET_TYPE = 'polygon-sticky'
NET_TYPES = ['polygon', 'polygon-sticky', 'clique']
CURRENT_NET_TYPE = DEFAULT_NET_TYPE

# Placer Frame
DEFAULT_FRAME_WIDTH = DEFAULT_WINDOW_WIDTH
DEFAULT_FRAME_HEIGHT = DEFAULT_WINDOW_HEIGHT
DEFAULT_CANVAS_BG = '#FAF8F8'
PLACER_FRAME_PANEL_WIDTH = 360
DEFAULT_PANEL_BG = '#FFFACD'
DEFAULT_GRID_PAD_X = 5
DEFAULT_GRID_PAD_Y = 5
DEFAULT_FRAME_OUTLINE_COLOR = 'white'
DEFAULT_FRAME_OUTLINE_WIDTH = 1.5
DEFAULT_FRAME_LEFT_WEIGHT = 1
DEFAULT_FRAME_RIGHT_WEIGHT = 2

DEFAULT_FRAME_RIGHT_FONT = 'Helvetica 14 bold'
DEFAULT_FRAME_LEFT_FONT = 'Helvetica 12'

DEFAULT_CANVAS_WIDTH = DEFAULT_FRAME_WIDTH - PLACER_FRAME_PANEL_WIDTH
DEFAULT_CANVAS_HEIGHT = DEFAULT_FRAME_HEIGHT


class Device:
    def __init__(self, device_id, canvas, objects, x, y,
                 half_width=DEFAULT_DEVICE_HALF_WIDTH,
                 half_height=DEFAULT_DEVICE_HALF_HEIGHT,
                 outline_width=DEFAULT_DEVICE_OUTLINE_WIDTH,
                 fill_color=DEFAULT_DEVICE_FILL_COLOR,
                 outline_color=DEFAULT_DEVICE_OUTLINE_COLOR,
                 selected_fill_color=DEFAULT_DEVICE_SELECTED_FILL_COLOR,
                 selected_outline_color=DEFAULT_DEVICE_SELECTED_OUTLINE_COLOR,
                 hover_fill_color=DEFAULT_DEVICE_HOVER_FILL_COLOR,
                 hover_outline_color=DEFAULT_DEVICE_HOVER_OUTLINE_COLOR):
        self.canvas = canvas
        self.rect = canvas.create_rectangle(x - half_width, y - half_height,
                                            x + half_width, y + half_height,
                                            fill=fill_color,
                                            outline=outline_color,
                                            activefill=hover_fill_color,
                                            activeoutline=hover_outline_color,
                                            width=outline_width,
                                            tags='device')
        self.id = device_id
        self.pins = list()
        self.half_width = half_width
        self.half_height = half_height
        self.center = (x, y)
        self.objects = objects
        self.fill_color = fill_color
        self.outline_color = outline_color
        self.selected_fill_color = selected_fill_color
        self.selected_outline_color = selected_outline_color
        self.hover_fill_color = hover_fill_color
        self.hover_outline_color = hover_outline_color
        self.selected = False

    def delete(self):
        # for pin in self.pins:
        #     pin.delete(from_device=True)
        #     self.objects.pop(self.objects.index(pin))
        self.canvas.delete(self.rect)
        self.pins.clear()

    def push_forward(self):
        self.canvas.tag_raise(self.rect, 'all')
        self.objects.append(self.objects.pop(self.objects.index(self)))

        for pin in self.pins:
            pin.push_forward(True)

    def is_selected(self):
        return self.selected

    def select(self):
        self.canvas.itemconfig(self.rect, fill=self.selected_fill_color, outline=self.selected_outline_color,
                               activefill=self.selected_fill_color, activeoutline=self.selected_outline_color)
        self.selected = True
        self.push_forward()

    def deselect(self):
        self.canvas.itemconfig(self.rect, fill=self.fill_color, outline=self.outline_color,
                               activefill=self.hover_fill_color, activeoutline=self.hover_outline_color)
        self.selected = False

    def add_pin(self, pin):
        self.pins.append(pin)

    def del_pin(self, pin):
        self.pins.pop(self.pins.index(pin))

    def move_relative(self, dx, dy):
        self.canvas.move(self.rect, dx, dy)
        self.center = (self.center[0] + dx, self.center[1] + dy)

        for pin in self.pins:
            pin.move_relative(dx, dy)

    def move(self, x, y):
        delta_x = x - self.center[0]
        delta_y = y - self.center[1]
        self.canvas.move(self.rect, delta_x, delta_y)
        self.center = (x, y)

        for pin in self.pins:
            pin.move_relative(delta_x, delta_y)

    def inside(self, x, y):
        return abs(x - self.center[0]) <= self.half_width and abs(y - self.center[1]) <= self.half_height

    def dist(self, x, y):
        return self.center[0] - x, self.center[1] - y

    def is_device(self):
        return True


class Pin:
    def __init__(self, pin_id, canvas, objects, x, y,
                 half_width=DEFAULT_PIN_HALF_WIDTH,
                 half_height=DEFAULT_PIN_HALF_HEIGHT,
                 outline_width=DEFAULT_PIN_OUTLINE_WIDTH,
                 fill_color=DEFAULT_PIN_FILL_COLOR,
                 outline_color=DEFAULT_PIN_OUTLINE_COLOR,
                 selected_fill_color=DEFAULT_PIN_SELECTED_FILL_COLOR,
                 selected_outline_color=DEFAULT_PIN_SELECTED_OUTLINE_COLOR,
                 hover_fill_color=DEFAULT_PIN_HOVER_FILL_COLOR,
                 hover_outline_color=DEFAULT_PIN_HOVER_OUTLINE_COLOR):
        self.canvas = canvas
        self.rect = canvas.create_rectangle(x - half_width, y - half_height,
                                            x + half_width, y + half_height,
                                            fill=fill_color,
                                            outline=outline_color,
                                            activefill=hover_fill_color,
                                            activeoutline=hover_outline_color,
                                            width=outline_width,
                                            tags='pin')
        self.id = pin_id
        self.half_width = half_width
        self.half_height = half_height
        self.center = (x, y)
        self.objects = objects
        self.assigned_device = None
        self.reassign_device()
        self.fill_color = fill_color
        self.outline_color = outline_color
        self.selected_fill_color = selected_fill_color
        self.selected_outline_color = selected_outline_color
        self.hover_fill_color = hover_fill_color
        self.hover_outline_color = hover_outline_color
        self.selected = False
        self.deleted = False

    def delete(self, from_device=False):
        if not from_device and self.assigned_device:
            self.assigned_device.del_pin(self)
        self.canvas.delete(self.rect)
        self.deleted = True

    def is_deleted(self):
        return self.deleted

    def push_forward(self, from_device=False):
        if self.assigned_device and not from_device:
            self.assigned_device.push_forward()
            self.canvas.tag_raise(self.rect, 'all')
        else:
            self.canvas.tag_raise(self.rect, 'all')
            self.objects.append(self.objects.pop(self.objects.index(self)))

    def is_selected(self):
        return self.selected

    def select(self):
        self.canvas.itemconfig(self.rect, fill=self.selected_fill_color, outline=self.selected_outline_color,
                               activefill=self.selected_fill_color, activeoutline=self.selected_outline_color)
        self.selected = True
        self.push_forward()

    def deselect(self):
        self.canvas.itemconfig(self.rect, fill=self.fill_color, outline=self.outline_color,
                               activefill=self.hover_fill_color, activeoutline=self.hover_outline_color)
        self.selected = False

    def move(self, x, y):
        delta_x = x - self.center[0]
        delta_y = y - self.center[1]
        self.canvas.move(self.rect, delta_x, delta_y)
        self.center = (x, y)

    def move_relative(self, dx, dy):
        self.canvas.move(self.rect, dx, dy)
        self.center = (self.center[0] + dx, self.center[1] + dy)

    def inside(self, x, y):
        return abs(x - self.center[0]) <= self.half_width and abs(y - self.center[1]) <= self.half_height

    def dist(self, x, y):
        return self.center[0] - x, self.center[1] - y

    def reassign_device(self):
        if self.assigned_device:
            self.assigned_device.del_pin(self)
            self.assigned_device = None
        self.assign_device()

    def assign_device(self):
        for d in self.objects:
            if d.is_device() and d.inside(self.center[0], self.center[1]):
                self.assigned_device = d

        if self.assigned_device:
            self.assigned_device.add_pin(self)

    def is_device(self):
        return False


class Net:
    def __init__(self, net_id, canvas, pins,
                 width=DEFAULT_NET_OUTLINE_WIDTH,
                 color=DEFAULT_NET_COLORS[0],
                 selected_width=DEFAULT_NET_SELECTED_OUTLINE_WIDTH,
                 selected_color=DEFAULT_NET_SELECTED_COLOR,
                 net_type=DEFAULT_NET_TYPE):
        self.id = net_id
        self.canvas = canvas
        self.pins = pins
        self.width = width
        self.color = color
        self.lines = []
        self.type = net_type
        self.selected_color = color
        self.selected_width = selected_width
        self.selected = False
        self.current_color = color
        self.current_width = width

        self.show()

    def delete(self):
        for line in self.lines:
            self.canvas.delete(line)

        self.lines.clear()
        self.pins.clear()

    def is_selected(self):
        return self.selected

    def select(self):
        self.selected = True
        self.current_color = self.selected_color
        self.current_width = self.selected_width

    def deselect(self):
        self.selected = False
        self.current_color = self.color
        self.current_width = self.width

    def on_line(self, x1, y1, x2, y2, x, y):
        print('on_line', x1, y1, x2, y2, x, y)
        if min(x1, x2) > x or max(x1, x2) < x:
            return False
        if min(y1, y2) > y or max(y1, y2) < y:
            return False
        if x1 == x2 or y1 == y2:
            return True
        k = (y2 - y1) / (x2 - x1)
        val = abs(y1 + k * (x - x1) - y)
        print('val:', val)
        return val < 2

    def draw_line(self, x1, y1, x2, y2):
        print('drawing line', x1, y1, x2, y2, self.current_width, self.current_color)
        line = self.canvas.create_line((x1, y1), (x2, y2),
                                       fill=self.current_color, width=self.current_width, tag='line')
        return line

    def on_border(self, x, y):
        ret = False
        print('on_border, id', self.id, x, y)
        print('len(lines)', len(self.lines))
        for line in self.lines:
            x1, y1, x2, y2 = self.canvas.coords(line)
            if self.on_line(x1, y1, x2, y2, x, y):
                ret = True
                break
        print('ret', ret)
        return ret

    def update(self):
        pins_to_delete = []
        for pin in self.pins:
            if pin.is_deleted():
                pins_to_delete.append(pin)

        for pin in pins_to_delete:
            self.pins.remove(pin)

        for line in self.lines:
            self.canvas.delete(line)

        self.lines.clear()

        self.show()

    def show(self):
        if self.type == 'polygon':
            self.show_polygon()
        elif self.type == 'polygon-sticky':
            self.show_polygon_sticky()
        elif self.type == 'clique':
            self.show_clique()
        else:
            print('Unknown type', self.type)

    def show_clique(self):
        n = len(self.pins)
        points = []
        for pin in self.pins:
            points.append(pin.center)

        for i in range(n):
            x1, y1 = points[i]
            for j in range(i + 1, n):
                x2, y2 = points[j]
                line = self.draw_line(x1, y1, x2, y2)
                self.lines.append(line)

    def show_polygon_sticky(self):
        points = []
        for pin in self.pins:
            points.append(pin.center)

        if len(points) <= 1:
            return
        elif len(points) == 2:
            line = self.draw_line(points[0][0], points[0][1], points[1][0], points[1][1])
            self.lines.append(line)
            return

        hull = ConvexHull(points=points)
        hull_points = []
        used = [0] * len(points)
        for i in hull.vertices:
            hull_points.append(points[i])
            used[i] = 1

        not_hull_points = []
        for i in range(len(points)):
            if not used[i]:
                not_hull_points.append(points[i])

        self.draw_hull(hull_points)
        self.draw_not_hull(not_hull_points, hull_points)

    def show_polygon(self):
        points = []
        for pin in self.pins:
            points.append(pin.center)

        if len(points) <= 1:
            return
        elif len(points) == 2:
            x1, y1 = points[0]
            x2, y2 = points[1]
            line = self.draw_line(x1, y1, x2, y2)
            self.lines.append(line)
            return

        hull = ConvexHull(points=points)
        hull_points = []
        for i in hull.vertices:
            hull_points.append(points[i])

        self.draw_hull(hull_points)

    def draw_hull(self, points):
        n = len(points)
        for i in range(n):
            a = points[i]
            b = points[(i + 1) % n]
            x1, y1 = a
            x2, y2 = b
            line = self.draw_line(x1, y1, x2, y2)
            self.lines.append(line)

    def dist2(self, x1, y1, x2, y2):
        dx = x2 - x1
        dy = y2 - y1
        return dx * dx + dy * dy

    def draw_not_hull(self, not_hull_points, hull_points):
        for x1, y1 in not_hull_points:
            closest = None
            best_dist = 1e9
            for x2, y2 in hull_points:
                if not closest or best_dist > self.dist2(x1, y1, x2, y2):
                    closest = (x2, y2)
                    best_dist = self.dist2(x1, y1, x2, y2)
            line = self.draw_line(x1, y1, closest[0], closest[1])
            self.lines.append(line)

    def add_pin(self, pin):
        self.pins.add(pin)

    def del_pin(self, pin):
        self.pins.remove(pin)

    def need_update(self):
        ret = False
        for pin in self.pins:
            if pin.is_selected() or pin.is_deleted() or (pin.assigned_device and pin.assigned_device.is_selected()):
                ret = True
                break
        return ret


def write_to_file(path_to_file, text):
    with open(path_to_file, 'w') as file:
        file.write(text)
    file.close()


class PlacerFrame(tk.Frame):
    def __init__(self, parent, root, frame_id, default_solver=None, params_cache=None, slug=None, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)
        self.pack(fill=tk.BOTH)
        self.frame_id = frame_id
        self.root = root
        self.slug = slug
        self.hidden = False

        if params_cache:
            self.params_cache = params_cache
        else:
            self.params_cache = dict()

        self.path_to_file = None
        self.parent = parent

        self.parent.update()
        self.pack(fill='both', expand=True)
        # [object]
        self.selected = set()
        # [net]
        self.selected_nets = set()

        self.canvas = tk.Canvas(self, width=self.winfo_width() - PLACER_FRAME_PANEL_WIDTH,
                                height=self.winfo_height(),
                                bg=DEFAULT_CANVAS_BG,
                                highlightbackground=DEFAULT_FRAME_OUTLINE_COLOR,
                                highlightthickness=0)
        self.canvas.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)

        self.update()

        self.panel = None
        self.grid_height = None
        self.solvers_label = None
        self.solvers = None
        self.selected_solver = None
        self.solvers_option_menu = None
        self.solve_button = None
        self.validate_and_estimate_result = None
        self.solved_count = dict()

        self.param_elems = list()
        self.result_elems = list()
        self.objects = list()
        self.nets = list()

        self.v_number_cmd = (self.register(self.validate_number_entry))

        self.init_panel(default_solver=default_solver)

        self.device_counter = 0
        self.pin_counter = 0
        self.net_counter = 0

        self.last_mouse_pos = (0, 0)

        self.key_a_pressed_time = 0

        # self.draw_grid()
        self.update()

    def cache_insert(self, key, value):
        self.params_cache[key] = value
    
    def cache_contains(self, key):
        return key in self.params_cache
    
    def cache_get(self, key):
        return self.params_cache[key]

    def get_closest(self, p, q):
        x, y, = p
        a, b = q
        if x % a < a - x % a:
            x -= x % a
        else:
            x += a - x % a

        if y % b < b - y % b:
            y -= y % b
        else:
            y += b - y % b

        return x, y

    def align_all(self, event=None):
        print('aligning...')
        for obj in self.objects:
            if obj.is_device() or (not obj.is_device() and not obj.assigned_device):
                x, y = self.get_closest(obj.center, (DEFAULT_ALIGN_X, DEFAULT_ALIGN_Y))
            else:
                x, y = self.get_closest(obj.center, (DEFAULT_ALIGN_PIN_X, DEFAULT_ALIGN_PIN_Y))     
            print('obj.center', obj.center)
            x, y = self.get_closest(obj.center, (DEFAULT_ALIGN_X, DEFAULT_ALIGN_Y))
            obj.move(x, y)
            print('new obj.center', (x, y))

        self.update_nets(all=True)
        self.canvas.update()

    def draw_grid(self):
        step_x = 5
        step_y = 5
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()

        for i in range(width // step_x):
            x = i * step_x
            self.canvas.create_line((x, 0), (x, height), fill='black', width=0.5)

        for i in range(height // step_y):
            y = i * step_y
            self.canvas.create_line((0, y), (width, y), fill='black', width=0.5)

    def validate_number_entry(self, p):
        return str.isnumeric(p.replace('.', '', 1)) or str(p) == ''

    def init_panel(self, default_solver=None):
        self.panel = tk.Frame(self, width=PLACER_FRAME_PANEL_WIDTH,
                              height=self.winfo_height(),
                              bg=DEFAULT_PANEL_BG,
                              highlightcolor=DEFAULT_FRAME_OUTLINE_COLOR,
                              highlightbackground=DEFAULT_FRAME_OUTLINE_COLOR,
                              highlightthickness=DEFAULT_FRAME_OUTLINE_WIDTH)
        self.panel.columnconfigure(0,
                                   minsize=PLACER_FRAME_PANEL_WIDTH // (
                                           DEFAULT_FRAME_LEFT_WEIGHT + DEFAULT_FRAME_RIGHT_WEIGHT) * DEFAULT_FRAME_LEFT_WEIGHT,
                                   weight=DEFAULT_FRAME_LEFT_WEIGHT)
        self.panel.columnconfigure(1,
                                   minsize=PLACER_FRAME_PANEL_WIDTH // (
                                           DEFAULT_FRAME_LEFT_WEIGHT + DEFAULT_FRAME_RIGHT_WEIGHT) * DEFAULT_FRAME_RIGHT_WEIGHT,
                                   weight=DEFAULT_FRAME_RIGHT_WEIGHT)
        self.panel.pack(side=tk.LEFT, fill=tk.Y)
        self.panel.grid_propagate(False)
        self.grid_height = 0

        self.solvers_label = tk.Label(self.panel, text='Solver:', font=DEFAULT_FRAME_RIGHT_FONT,
                                      bg=DEFAULT_PANEL_BG)

        self.solvers = placer.solvers()

        need_update = False

        if default_solver == None:
            default_solver = self.solvers[0]
            need_update = True

        solvers_variable = tk.StringVar(self.panel)
        solvers_variable.set(default_solver)
        self.solvers_option_menu = tk.OptionMenu(self.panel, solvers_variable,
                                                 *self.solvers, command=self.update_solver_info)
        # self.solvers_option_menu.config(bg=DEFAULT_PANEL_BG, font='Helvetica 14', width=1)
        self.solvers_option_menu.config(bg='white', font='Helvetica 14', width=1)
        self.add_to_grid(self.solvers_label, self.solvers_option_menu)

        # if need_update:
        self.update_solver_info(default_solver)

    def clear_param_elems(self):
        for label, entry in self.param_elems:
            label.grid_forget()
            entry.grid_forget()

    def clear_validate_and_estimate_result(self):
        if self.validate_and_estimate_result:
            self.validate_and_estimate_result[0].grid_forget()
            self.validate_and_estimate_result[1].grid_forget()

    def clear_solve_button(self):
        if self.solve_button:
            self.solve_button.grid_forget()

    def clear_result_elems(self):
        print('clear result elems', len(self.result_elems), self.result_elems)
        for label_left, label_right in self.result_elems:
            label_left.grid_forget()
            label_right.grid_forget()

    def clear_panel(self):
        self.clear_param_elems()
        self.clear_validate_and_estimate_result()
        self.clear_solve_button()
        self.clear_result_elems()

    def draw_panel(self):
        self.grid_height = 1  # solver / select solver

        print('drawing panel')
        print('param_elems.size', len(self.param_elems))

        for label, entry in self.param_elems:
            self.add_to_grid(label, entry)

        if self.validate_and_estimate_result:
            self.add_to_grid(self.validate_and_estimate_result[0],
                             self.validate_and_estimate_result[1])

        if self.solve_button:
            self.solve_button.grid(row=self.grid_height, column=0,
                                   columnspan=2,
                                   padx=DEFAULT_GRID_PAD_X,
                                   pady=DEFAULT_GRID_PAD_Y,
                                   sticky=tk.EW)
            self.grid_height += 1

        print('result_elems.size', self.result_elems)

        for label_left, label_right in self.result_elems:
            self.add_to_grid(label_left, label_right)

    def validate_and_estimate(self):
        input_path, output_path, params = self.create_temp_files()
        print('params', params)
        print('validate and estimate...')
        text, value, _ = placer.validate_and_estimate(self.selected_solver, input_path, output_path, **params)[0]
        print('validated and estimated...')
        print('remove_input', os.remove(input_path))
        print('remove_output', os.remove(output_path))
        print('removed files')
        return text + ':', value

    def update_widget_vae(self, text, value):
        self.validate_and_estimate_result[0].config(text=text)
        self.validate_and_estimate_result[1].config(text=value)

    def update_validate_and_estimate_result(self, event=None):
        text, value = self.validate_and_estimate()
        self.update_widget_vae(text, value)

    def update_solver_info(self, solver_name):
        if solver_name == self.selected_solver:
            return

        self.clear_panel()

        self.selected_solver = solver_name

        # init param_elems
        self.param_elems.clear()

        self.solver_params = placer.params(self.selected_solver)

        # update solver params
        for name, value, _ in self.solver_params:
            self.current_label = tk.Label(self.panel, text=name + ':',
                                    font=DEFAULT_FRAME_RIGHT_FONT,
                                    bg=DEFAULT_PANEL_BG)
            self.current_entry = tk.Entry(self.panel,
                                    font='Helvetica 14',
                                    highlightbackground=DEFAULT_PANEL_BG,
                                    justify='center',
                                    validate='all',
                                    validatecommand=(self.v_number_cmd, '%P'))
            self.current_entry.bind('<Return>', self.update_validate_and_estimate_result)
            self.current_entry.bind('<FocusOut>', self.update_validate_and_estimate_result)
            self.current_entry.bind('<FocusIn>', self.update_validate_and_estimate_result)
            
            if self.cache_contains(name):
                value = self.cache_get(name)

            self.current_entry.insert(0, value)
            self.param_elems.append((self.current_label, self.current_entry))
            self.current_entry = None
            self.current_label = None

        # init validate_and_estimate_result
        text, value = self.validate_and_estimate()
        self.validate_and_estimate_result = (tk.Label(self.panel, text=text,
                                                      font=DEFAULT_FRAME_RIGHT_FONT,
                                                      bg=DEFAULT_PANEL_BG),
                                             tk.Label(self.panel, text=value,
                                                      font='Helvetica 14',
                                                      bg=DEFAULT_PANEL_BG))
        self.solve_button = tk.Button(self.panel,
                                      text='Solve',
                                      font=DEFAULT_FRAME_RIGHT_FONT,
                                      bg='white',
                                      highlightthickness=0.5,
                                      highlightbackground=DEFAULT_PANEL_BG,
                                      highlightcolor='blue',
                                      borderwidth=5,
                                      relief='raised')
        self.solve_button.bind('<ButtonPress>', self.call_solver)

        # init result elems
        self.clear_result_elems()
        self.result_elems.clear()

        # draw
        self.draw_panel()

    def add_result_elems(self, result_list):
        for text, value, _ in result_list:
            self.result_elems.append(
                (
                    tk.Label(self.panel, text=text + ':',
                             font=DEFAULT_FRAME_RIGHT_FONT,
                             bg=DEFAULT_PANEL_BG),
                    tk.Label(self.panel, text=value,
                             font='Helvetica 14',
                             bg=DEFAULT_PANEL_BG)
                )
            )

    def add_param_elems(self, params_list, flag=False):
        for name in params_list:
            value = params_list[name]
            print('(name, value)')
            self.current_label = tk.Label(self.panel, text=name + ':',
                                    font=DEFAULT_FRAME_RIGHT_FONT,
                                    bg=DEFAULT_PANEL_BG)
            self.current_entry = tk.Entry(self.panel,
                                    font='Helvetica 14',
                                    highlightbackground=DEFAULT_PANEL_BG,
                                    justify='center',
                                    validate='all',
                                    validatecommand=(self.v_number_cmd, '%P'))
            self.current_entry.bind('<Return>', self.update_validate_and_estimate_result)
            self.current_entry.bind('<FocusOut>', self.update_validate_and_estimate_result)
            self.current_entry.bind('<FocusIn>', self.update_validate_and_estimate_result)
            self.current_entry.insert(0, value)
            self.param_elems.append((self.current_label, self.current_entry))
            self.current_entry = None
            self.current_label = None

    def update_cache(self, params_dict):
        for key in params_dict:
            self.cache_insert(key, params_dict[key])

    def draw_graph(self, result):
        t = list()
        twls = list()
        best_twl = None
        for name, val, _ in result:
            if len(name) >= 2 and name[:2] == 'di':
                t.append(float(name[3:]))
                twl = float(val)
                twls.append(twl)
                if not best_twl or best_twl > twl:
                    best_twl = twl

        if len(t) == 0 or best_twl == 0:
            return
        
        twl_dev = [(x - best_twl) / best_twl * 100 for x in twls]

        plt.title('Graph')
        plt.plot(t, twl_dev)

        name = self.big_random_name(suffix='.png')

        plt.savefig(name)

        plt.close()

        # im = Image.open(name)
        # im.show()

        # os.remove(name)
         

    def call_solver(self, event=None):
        self.update_validate_and_estimate_result()

        print('call_solver validate',
              self.validate_and_estimate_result[0]['text'], self.validate_and_estimate_result[1]['text'])
        if not self.validate_and_estimate_result or self.validate_and_estimate_result[0]['text'] == 'Error:':
            return


        input_path, output_path, params_dict = self.create_temp_files()
        self.update_cache(params_dict)

        if self.selected_solver not in self.solved_count:
            self.solved_count[self.selected_solver] = 0
        
        self.solved_count[self.selected_solver] += 1
        result_list = placer.solve(self.selected_solver, input_path, output_path, **params_dict)

        self.draw_graph(result_list)


        print('after solver call', self.selected_solver)

        output_tab = self.root.add_tab(slug=self.slug + '_' + self.selected_solver + str(self.solved_count[self.selected_solver]), 
                                       default_solver=self.selected_solver,
                                       params_cache=self.params_cache.copy())
        output_tab.init_from_file(output_path)
        output_tab.clear_panel()
        
        print('start initing after solver call')

        output_tab.solver_params = placer.params(self.selected_solver)
        print('params dict', self.get_params_dict())
        output_tab.param_elems.clear()
        output_tab.add_param_elems(self.get_params_dict())
        output_tab.add_result_elems(result_list)
        output_tab.update_widget_vae(self.validate_and_estimate_result[0]['text'], self.validate_and_estimate_result[1]['text'])
        output_tab.draw_panel()
        os.remove(input_path)
        os.remove(output_path)
        self.parent.select(output_tab)

    def big_random_name(self, suffix=''):
        left = int(1e8)
        right = int(1e9) - 1
        return '__name_' \
            + '_' + str(random.randint(left, right)) \
            + '_' + str(random.randint(left, right)) \
            + '_' + str(random.randint(left, right)) \
            + '_' + str(random.randint(left, right)) \
            + '_' + str(random.randint(left, right)) \
            + '_' + suffix

    def get_params_dict(self):
        params_dict = dict()

        for label, entry in self.param_elems:
            text = label['text']
            text_len = len(text)
            params_dict[text[0:text_len - 1]] = entry.get()

        return params_dict

    def create_temp_files(self):
        random_name = self.big_random_name()
        temp_input = random_name + '_input.txt'
        temp_output = random_name + '_output.txt'

        write_to_file(temp_input, self.get_layout_info())
        write_to_file(temp_output, 'output.txt')

        return temp_input, temp_output, self.get_params_dict()

    def add_to_grid(self, left, right,
                    pad_x_left=DEFAULT_GRID_PAD_X,
                    pad_y_left=DEFAULT_GRID_PAD_Y,
                    pad_x_right=DEFAULT_GRID_PAD_X,
                    pad_y_right=DEFAULT_GRID_PAD_Y):
        left.grid(row=self.grid_height, column=0, sticky=tk.W, padx=pad_x_left, pady=pad_y_left)
        right.grid(row=self.grid_height, column=1, sticky=tk.EW, padx=pad_x_right, pady=pad_y_right)
        self.grid_height += 1

    def get_mapper(self, x):
        if len(x) == 0:
            return list()
        x.sort()
        ret = [0] * (max(x) + 1)
        for i in range(len(x)):
            ret[x[i]] = i
        return ret

    def get_layout_info(self):
        ret = ''

        devices_id = list()
        assigned_pins_id = list()
        good_nets_id = list()

        for obj in self.objects:
            if obj.is_device():
                devices_id.append(obj.id)
            elif not obj.is_device() and obj.assigned_device:
                assigned_pins_id.append(obj.id)

        for net in self.nets:
            assigned_pins_count = 0
            for pin in net.pins:
                if pin.assigned_device:
                    assigned_pins_count += 1
            if assigned_pins_count >= 2:
                good_nets_id.append(net.id)

        device_map = self.get_mapper(devices_id)
        pins_map = self.get_mapper(assigned_pins_id)
        nets_map = self.get_mapper(good_nets_id)

        ret += 'Devices\n'
        ret += str(len(devices_id)) + '\n'
        for obj in self.objects:
            if obj.is_device():
                ret += str(device_map[obj.id]) + '\n' + str(obj.center[0]) + ' ' + str(obj.center[1]) + ' ' + \
                       str(obj.half_width) + ' ' + str(obj.half_height) + '\n'

        ret += 'Pins\n'
        ret += str(len(assigned_pins_id)) + '\n'
        for obj in self.objects:
            if not obj.is_device() and assigned_pins_id.count(obj.id) > 0 and device_map.count(
                    obj.assigned_device.id) > 0:
                ret += str(pins_map[obj.id]) + '\n' + str(device_map[obj.assigned_device.id]) + ' ' + \
                       str(obj.center[0] - obj.assigned_device.center[0]) \
                       + ' ' + str(obj.center[1] - obj.assigned_device.center[1]) \
                       + ' ' + str(obj.half_width) + ' ' + str(obj.half_height) + '\n'

        ret += 'Nets\n'
        ret += str(len(good_nets_id)) + '\n'
        for net in self.nets:
            if net.id not in good_nets_id:
                continue
            ret += str(nets_map[net.id]) + '\n'
            assigned_pins = list()
            for pin in net.pins:
                if pin.assigned_device:
                    assigned_pins.append(pin)
            net_size = len(assigned_pins)
            ret += str(net_size) + ' '
            for i in range(net_size):
                ret += str(pins_map[assigned_pins[i].id])
                if i + 1 < net_size:
                    ret += ' '
            ret += '\n'

        self.update()
        self.canvas.update()
        screen_width, screen_height = self.canvas.winfo_width(), self.canvas.winfo_height()
        ret += str(screen_width) + ' ' + str(screen_height) + '\n'

        return ret

    def clear_all(self):
        print('clear all size selected1', len(self.selected), len(self.selected_nets))
        print('clear all size object1', len(self.objects), len(self.nets))

        for obj in self.objects:
            self.canvas.delete(obj.rect)

        for net in self.nets:
            net.delete()

        print('clear all size selected2', len(self.selected), len(self.selected_nets))
        print('clear all size object2', len(self.objects), len(self.nets))

        self.canvas.update()
        self.objects.clear()
        self.selected.clear()
        self.nets.clear()
        self.selected_nets.clear()

        self.device_counter = 0
        self.pin_counter = 0
        self.net_counter = 0

    def init_from_file(self, path_to_file):
        file = open(path_to_file, 'r')

        file.readline()  # devices
        device_count = int(file.readline())

        devices = [0] * device_count

        for i in range(device_count):
            device_id = int(file.readline())  # id

            center_x, center_y, half_width, half_height = [int(x) for x in file.readline().strip().split(' ')]
            self.create_device_by_coords(center_x, center_y, half_width, half_height)
            devices[device_id] = (center_x, center_y)

        file.readline()  # pins
        pin_count = int(file.readline())

        pins = [0] * pin_count

        for i in range(pin_count):
            pin_id = int(file.readline())  # id

            assigned_device_id, dx, dy, half_width, half_height = [int(x) for x in file.readline().strip().split(' ')]
            center_x, center_y = devices[assigned_device_id][0] + dx, devices[assigned_device_id][1] + dy

            pins[pin_id] = self.create_pin_by_coords(center_x, center_y, half_width, half_height)

        file.readline()  # nets
        net_count = int(file.readline())

        for i in range(net_count):
            file.readline()  # id

            pin_ids = [int(x) for x in file.readline().strip().split(' ')]
            current_pins = list()
            for id in pin_ids[1:]:
                current_pins.append(pins[id])

            self.add_net_by_pins(current_pins)

        file.close()

    def gen_random_name(self):
        return 'layout' + str(random.randint(int(1e8), int(1e9 - 1)))

    def update_nets(self, all=False):
        for net in self.nets:
            print('updating net', net.id)
            if all or net.need_update():
                net.update()
        self.canvas.tag_raise('line')

    def add_net_by_pins(self, pins):
        print('Add net by pins', [pin.center for pin in pins])

        if len(pins) == 0:
            return None

        current_net = Net(self.net_counter, self.canvas, pins,
                          color=DEFAULT_NET_COLORS[self.net_counter % len(DEFAULT_NET_COLORS)])
        self.net_counter += 1
        self.nets.append(current_net)
        self.canvas.tag_raise('line')

        return current_net

    def add_net(self, event=None):
        if len(self.selected) == 0:
            return

        pins = set()
        for obj in self.selected:
            if obj.is_device():
                for pin in obj.pins:
                    pins.add(pin)
            else:
                pins.add(obj)

        if len(pins) == 0:
            return None

        current_net = self.add_net_by_pins(pins)

        self.update_validate_and_estimate_result()

        return current_net

    def key_a_press(self, event=None):
        self.key_a_pressed_time = time.time()

    def delete_selected(self, event=None):
        print('Deleting')
        obj_list = list()
        for obj in self.objects:
            obj_list.append(obj)

        for obj in obj_list:
            if obj.is_device():
                if obj.is_selected():
                    obj.delete()
                    self.objects.pop(self.objects.index(obj))
            else:
                if obj.is_selected() or (obj.assigned_device and obj.assigned_device.is_selected()):
                    obj.delete(from_device=True)
                    self.objects.pop(self.objects.index(obj))

        for obj in self.selected_nets:
            self.nets.pop(self.nets.index(obj))
            obj.delete()

        self.update_nets()

        self.deselect()

        self.update_validate_and_estimate_result()

    def deselect(self):
        print('Deselecting')
        for obj in self.selected:
            obj.deselect()
        for obj in self.selected_nets:
            obj.deselect()
            obj.update()

        self.selected.clear()
        self.selected_nets.clear()

        print('len(selected_nets)', len(self.selected_nets))

    def get_currently_selected(self, x, y):
        currently_selected = None
        for d in self.objects:
            if d.inside(x, y):
                currently_selected = d
        return currently_selected

    def get_currently_selected_net(self, x, y):
        currently_selected_net = None
        for n in self.nets:
            if n.on_border(x, y):
                currently_selected_net = n
        return currently_selected_net

    def b1_press(self, event):
        print('Pressed', event.x, event.y)

        self.last_mouse_pos = (event.x, event.y)

        if time.time() - self.key_a_pressed_time > 0.4:
            self.deselect()

        currently_selected = self.get_currently_selected(event.x, event.y)

        if currently_selected and currently_selected not in self.selected:
            self.selected.add(currently_selected)
            currently_selected.select()
        elif currently_selected and currently_selected in self.selected:
            self.selected.remove(currently_selected)
            currently_selected.deselect()
        elif not currently_selected:
            currently_selected_net = self.get_currently_selected_net(event.x, event.y)
            print('currently_selected_net', currently_selected_net)

            if currently_selected_net and currently_selected_net not in self.selected_nets:
                print('selecting currently_selected_net')
                self.selected_nets.add(currently_selected_net)
                currently_selected_net.select()
            elif currently_selected_net and currently_selected_net in self.selected_nets:
                self.selected_nets.remove(currently_selected_net)
                currently_selected_net.deselect()

        self.update_nets()
        for net in self.selected_nets:
            net.update()

    def b1_motion(self, event):
        print('Moved to', event.x, event.y)

        dx, dy = event.x - self.last_mouse_pos[0], event.y - self.last_mouse_pos[1]
        self.last_mouse_pos = (event.x, event.y)

        print('selected.size =', len(self.selected))

        for obj in self.selected:
            if not obj.is_device() and obj.assigned_device and obj.assigned_device.is_selected():
                continue
            obj.move_relative(dx, dy)

        self.update_nets()

    def b1_release(self, event):
        print('Released', event.x, event.y)

        self.last_mouse_pos = (event.x, event.y)

        for obj in self.selected:
            if not obj.is_device():
                obj.reassign_device()

    def create_device_by_coords(self, x, y,
                                half_width=DEFAULT_DEVICE_HALF_WIDTH,
                                half_height=DEFAULT_DEVICE_HALF_HEIGHT):
        created_device = Device(self.device_counter, self.canvas, self.objects,
                                x, y, half_width=half_width, half_height=half_height)
        self.device_counter += 1

        self.objects.append(created_device)
        self.canvas.tag_raise('line', 'all')

        self.canvas.update()

        self.update_validate_and_estimate_result()

        return created_device

    def create_device(self, event,
                      half_width=DEFAULT_DEVICE_HALF_WIDTH,
                      half_height=DEFAULT_DEVICE_HALF_HEIGHT):

        print('create device, frame:', self.frame_id)

        x = self.canvas.winfo_pointerx() - self.canvas.winfo_rootx()
        y = self.canvas.winfo_pointery() - self.canvas.winfo_rooty()

        if x <= self.canvas.winfo_width() and y <= self.canvas.winfo_height():
            print('Create device', x, y)
            self.canvas.focus_set()
            return self.create_device_by_coords(x, y, half_width, half_height)
        else:
            return None

    def create_pin_by_coords(self, x, y,
                             half_width=DEFAULT_PIN_HALF_WIDTH,
                             half_height=DEFAULT_PIN_HALF_HEIGHT):
        created_pin = Pin(self.pin_counter, self.canvas, self.objects,
                          x, y, half_width=half_width, half_height=half_height)
        self.pin_counter += 1

        self.objects.append(created_pin)
        self.canvas.tag_raise('line', 'all')

        self.canvas.update()

        self.update_validate_and_estimate_result()

        return created_pin

    def create_pin(self, event,
                   half_width=DEFAULT_PIN_HALF_WIDTH,
                   half_height=DEFAULT_PIN_HALF_HEIGHT):

        print('create pin, frame:', self.frame_id)

        x = self.canvas.winfo_pointerx() - self.canvas.winfo_rootx()
        y = self.canvas.winfo_pointery() - self.canvas.winfo_rooty()

        if x <= self.canvas.winfo_width() and y <= self.canvas.winfo_height():
            print('Create pin', x, y)
            self.canvas.focus_set()
            return self.create_pin_by_coords(x, y, half_width, half_height)
        else:
            return None


class PlacerGUI(tk.Tk):
    def __init__(self):
        super().__init__()

        self.geometry(DEFAULT_GEOM)
        self.title(WINDOW_TITLE)

        self.file_menu = None
        self.edit_menu = None
        self.settings_menu = None

        self.main_menu = None
        self.init_menu()

        self.notebook = None
        self.tabs = list()
        self.open_tabs = 0
        self.init_notebook()

        self.add_tab()

        self.init_frame_bind()


    def add_tab(self, slug=None, default_solver=None, params_cache=None):
        self.open_tabs += 1
        frame_id = len(self.tabs)
        if not slug:
            slug = "unsaved" + str(frame_id + 1)
        current_frame = PlacerFrame(self.notebook, self, frame_id=frame_id, slug=slug, default_solver=default_solver, width=DEFAULT_FRAME_WIDTH,
                                        height=DEFAULT_FRAME_HEIGHT,
                                        params_cache=params_cache)
        self.notebook.add(current_frame, text=current_frame.slug)
        self.tabs.append(current_frame)
        return current_frame

    def get_current_frame(self):
        if self.notebook.select():
            idx = self.notebook.index('current')
            return self.tabs[idx]
        else:
            return None
        
    def get_next_frame(self):
        tabs_cnt = len(self.tabs)
        i = (self.tabs.index(self.get_current_frame()) + 1) % tabs_cnt
        while self.tabs[i].hidden:
            i += 1
            i %= tabs_cnt
        return self.tabs[i]

    def get_prev_frame(self):
        tabs_cnt = len(self.tabs)
        i = (self.tabs.index(self.get_current_frame()) - 1 + tabs_cnt) % tabs_cnt
        while self.tabs[i].hidden:
            i += -1 + tabs_cnt
            i %= tabs_cnt
        return self.tabs[i]

    def init_notebook(self):
        self.notebook = ttk.Notebook()
        self.notebook.pack(expand=True, fill=tk.BOTH)

    def init_frame_bind(self):
        self.bind('D', lambda x: self.get_current_frame().create_device(x))
        self.bind('d', lambda x: self.get_current_frame().create_device(x))

        self.bind('f', lambda x: self.get_current_frame().create_pin(x))
        self.bind('F', lambda x: self.get_current_frame().create_pin(x))

        self.bind('<BackSpace>', lambda x: self.get_current_frame().delete_selected(x))
        self.bind('<Delete>', lambda x: self.get_current_frame().delete_selected(x))

        self.bind('s', lambda x: self.get_current_frame().add_net(x))
        self.bind('S', lambda x: self.get_current_frame().add_net(x))

        self.bind('a', lambda x: self.get_current_frame().key_a_press(x))
        self.bind('A', lambda x: self.get_current_frame().key_a_press(x))

        # self.bind('b', lambda x: self.get_current_frame().align_all(x))
        # self.bind('B', lambda x: self.get_current_frame().align_all(x))

        self.bind('a', lambda x: self.get_current_frame().key_a_press(x))
        self.bind('A', lambda x: self.get_current_frame().key_a_press(x))

        self.bind('<Control-w>', lambda x: self.close_tab())
        self.bind('<Control-W>', lambda x: self.close_tab())

        self.bind('<Control-t>', lambda x: self.create_new_tab())
        self.bind('<Control-T>', lambda x: self.create_new_tab())

        self.bind('<Control-s>', lambda x: self.save_file())
        self.bind('<Control-S>', lambda x: self.save_file())

        self.bind('<Control-q>', lambda x: self.on_closing())
        self.bind('<Control-Q>', lambda x: self.on_closing())

        self.bind('<Control-o>', lambda x: self.open_file())
        self.bind('<Control-O>', lambda x: self.open_file())

        self.bind('<Control-Left>', lambda x: self.notebook.select(self.get_prev_frame()))
        self.bind('<Control-Right>', lambda x: self.notebook.select(self.get_next_frame()))

        self.bind('<ButtonPress>', lambda x: self.get_current_frame().b1_press(x))
        self.bind('<B1-Motion>', lambda x: self.get_current_frame().b1_motion(x))
        self.bind('<ButtonRelease>', lambda x: self.get_current_frame().b1_release(x))

        self.protocol("WM_DELETE_WINDOW", self.on_closing)

    def on_closing(self, event=None):
        if messagebox.askokcancel("Quit", "All unsaved changes fill be losed"):
            self.destroy()

    def init_menu(self):
        self.main_menu = tk.Menu(self)
        self.config(menu=self.main_menu)

        self.file_menu = tk.Menu(self.main_menu, tearoff=0)
        self.file_menu.add_command(label='Open...', command=self.open_file)
        self.file_menu.add_command(label='Save', command=self.save_file)
        self.file_menu.add_command(label='Save as...', command=self.save_file_as)
        self.file_menu.add_command(label='New', command=self.create_new_tab)
        self.file_menu.add_command(label='Close', command=self.close_tab)

        self.edit_menu = tk.Menu(self.main_menu, tearoff=0)
        self.edit_menu.add_command(label='Clear all', command=self.ask_clear_all)
        self.edit_menu.add_command(label='Undo')
        self.edit_menu.add_command(label='Redo')

        self.settings_menu = tk.Menu(self.main_menu, tearoff=0)
        self.settings_menu.add_command(label='Colors')
        self.settings_menu.add_command(label='Nets')

        self.main_menu.add_cascade(label='File', menu=self.file_menu)
        self.main_menu.add_cascade(label='Edit', menu=self.edit_menu)
        self.main_menu.add_cascade(label='Settings', menu=self.settings_menu)

    def ask_clear_all(self):
        res = messagebox.askyesno('Clear all', 'Are you sure?')
        if res:
            self.get_current_frame().clear_all()

    def cut_path_to_file(self, s):
        idx_last_slash = -1
        idx_last_dot = len(s)
        for i in range(len(s)):
            if s[i] == '/' or s[i] == '\\':
                idx_last_slash = i
            elif s[i] == '.':
                idx_last_dot = i
        return s[idx_last_slash+1:idx_last_dot]
    
    def open_file(self):
        path_to_file = tkinter.filedialog.askopenfilename()
        if path_to_file:
            added_frame = self.add_tab(slug=self.cut_path_to_file(str(path_to_file)))
            added_frame.init_from_file(path_to_file)
            added_frame.path_to_file = path_to_file
            self.notebook.select(added_frame)

    def save_file(self):
        current_frame = self.get_current_frame()
        if not current_frame.path_to_file:
            return self.save_file_as()
        else:
            write_to_file(current_frame.path_to_file, current_frame.get_layout_info())
            return True

    def save_file_as(self):
        path_to_file = tkinter.filedialog.asksaveasfilename()
        if path_to_file:
            current_frame = self.get_current_frame()
            write_to_file(path_to_file, current_frame.get_layout_info())
            current_frame.path_to_file = path_to_file
            current_frame.slug = self.cut_path_to_file(str(path_to_file))
            self.notebook.tab(current_frame, text=current_frame.slug)
            return True
        else:
            return False

    def create_new_tab(self):
        created_tab = self.add_tab()
        self.notebook.select(created_tab)

    def close_tab(self):
        if self.open_tabs == 1:
            return
        else:
            res = messagebox.askyesnocancel('Close tab', 'Save?')
            if res == True:
                if self.save_file():
                    current_frame = self.get_current_frame()
                    self.notebook.hide(current_frame)
                    current_frame.hidden = True
                    self.open_tabs -= 1
                else:
                    return
            elif res == None:
                return
            else:
                current_frame = self.get_current_frame()
                self.notebook.hide(current_frame)
                current_frame.hidden = True
                self.open_tabs -= 1


if __name__ == '__main__':
    app = PlacerGUI()
    photo = tk.PhotoImage(file = 'cowboy4.png')
    app.wm_iconphoto(True, photo)
    app.mainloop()
