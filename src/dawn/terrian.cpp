#include "terrian.h"


terrian::terrian() {

	update_projection = false;
	update_cam = false;
	projection = glm::mat4(1.0f);
    view = glm::mat4(1.0f);
    draw_mode = -1;
	cube_shader = NULL;
	draw_selected = false;
	closedList = NULL;

	gen_orders = new std::vector< work_order*>;
	Time = NULL;
	deltatime = NULL;

	meeting_zone = NULL;
}

void terrian::draw() {
    switch (draw_mode)
    {
    case 0:
        draw_space();
        break;
    case 1:
        draw_cubes();
        break;
    default:
        std::cout << "no draw type init" << std::endl;
        break;
    }
}

void terrian::draw_selection(Shader* shade) {
	updateBuffer_ter();

	shade->use();
	shade->setMat4("projection", projection);
	shade->setMat4("view", view);
	shade->setInt("texture_diffuse1", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube->textures_loaded[0].id);
	shade->setBool("u_color1", true);
	shade->setBool("u_color2", false);
	shade->setBool("u_color2", false);
	if (cube_amount - 255 <= 0) {
		for (unsigned int i = 0; i < cube->meshes.size(); i++)
		{
			glBindVertexArray(cube->meshes[i].VAO);
			glDrawElementsInstanced(GL_TRIANGLES, cube->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, cube_amount);
			glBindVertexArray(0);
		}
	}
	else {
		int start = 0;
		int render = 254;
		int total = 254;
		int increment = total;
		int remaining = cube_amount;


		while (remaining > 0) {

			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, total * sizeof(glm::mat4), &cube_matrices[start], GL_STATIC_DRAW);

			for (unsigned int i = 0; i < cube->meshes.size(); i++)
			{
				glBindVertexArray(cube->meshes[i].VAO);
				glDrawElementsInstanced(GL_TRIANGLES, cube->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, render);
				glBindVertexArray(0);
			}

			start += render;
			remaining -= render;
			if (remaining < 255) {
				render = remaining;
				total = remaining;
			}
			//std::cout << remaining << std::endl;
			shade->setBool("u_color2", true);
		}
		//std::cout << "out of loop" << std::endl;

	}
}

void terrian::update() {
	switch (draw_mode)
	{

	case 1:
		update_cubes();
		break;
	default:
		std::cout << "no update for this type type init" << std::endl;
		break;
	}
}

void terrian::update_cubes() {
	static float pasted_time = 0;
	static float y = 0;
	pasted_time += (*deltatime);

	if (pasted_time >= (3.14159 * 15)) {
		y = 0;
		pasted_time = 0;
	}
	else {
		y += (*deltatime);
	}
	float x = 0;
	float z = 0;

	for (unsigned int i = 0; i < cube_amount; i++) {
		glm::mat4 model = glm::mat4(1.0f);
		// model = glm::translate(model, glm::vec3(x, sin((x+ pasted_time)), z));
	   // model = glm::translate(model, glm::vec3(x, sin(pasted_time - (pasted_time / x)), z));
		model = glm::translate(model, glm::vec3(x, sin(pasted_time - (pasted_time / x) - (pasted_time / z)), z));
		cube_matrices[i] = model;
		x += cube_offset;

		if (x == (cube_offset * x_width)) {
			z += cube_offset;
			x = 0;
		}
	}
}

void terrian::update_zones() {
	if (Time->is_day()) {
		farm_zone->update(*deltatime);
	}
	if (items_to_add != NULL) {
		//generate agriculture work orders
		for (int i = 0; i < farm_tiles_need_work->size(); i++) {
			work_jobs job_to_do;
			if (!farm_tiles_need_work[0][i]->tilled) {
				job_to_do = TILL_SOIL;
			}
			else {
				switch (farm_tiles_need_work[0][i]->tending_action) {
				case 2:
					job_to_do = TEND_PLANT;
					break;
				case 3:
					job_to_do = HARVEST_PLANT;
					break;
				default:
					break;
				}
				
			}
			gen_orders->push_back(generate_work_order(job_to_do,
				farm_tiles_need_work[0][i]->loc->x, 5, farm_tiles_need_work[0][i]->loc->z, farm_tiles_need_work[0][i]));
		}
		farm_tiles_need_work->clear();
	}
	else {
		std::cout << "the was a problem getting a list of farm tiles" << std::endl;
	}

	if (items_to_add != NULL) {
		//see if it has to spawn in any items from a farm
		for (int i = 0; i < items_to_add->size(); i++) {
			//spawn the item in the world
			item_info* temp = spawn_item(LOG_T, items_to_add[0][i]->x, items_to_add[0][i]->z);
			//then generate a work order for the object
			if (temp != NULL) {
				gen_orders->push_back(generate_work_order(SACRIFICE_OBJ, items_to_add[0][i]->x, 5, items_to_add[0][i]->z));
			}
		}
		items_to_add->clear();
	}
	else {
		std::cout << "the was a problem getting a list of grown items" << std::endl;
	}
}

void terrian::draw_cubes() {
    updateBuffer_ter();

    cube_shader->use();
    cube_shader->setMat4("projection", projection);
    cube_shader->setMat4("view", view);
    cube_shader->setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cube->textures_loaded[0].id);
    for (unsigned int i = 0; i < cube->meshes.size(); i++)
    {
        glBindVertexArray(cube->meshes[i].VAO);
        glDrawElementsInstanced(GL_TRIANGLES, cube->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, cube_amount);
        glBindVertexArray(0);
    }

}

void terrian::draw_space() {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    asteroidShader->use();
    asteroidShader->setMat4("projection", projection);
    asteroidShader->setMat4("view", view);
    planetShader->use();
    planetShader->setMat4("projection", projection);
    planetShader->setMat4("view", view);

    // draw planet
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
    model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
    planetShader->setMat4("model", model);
    planet->Draw(planetShader);

    // draw meteorites
    asteroidShader->use();
    asteroidShader->setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rock->textures_loaded[0].id); // note: we also made the textures_loaded vector public (instead of private) from the model class.
    for (unsigned int i = 0; i < rock->meshes.size(); i++)
    {
        glBindVertexArray(rock->meshes[i].VAO);
        glDrawElementsInstanced(GL_TRIANGLES, rock->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount);
        glBindVertexArray(0);
    }

}

void terrian::cubes_init() {
	std::cout << "creating the terrian class (cubes)" << std::endl;
	if (Time != NULL) {
		deltatime = Time->get_time_change();
	}
	else {
		std::cout << "there was a problem getting time in the sky" << std::endl;
		while (true);
	}
	draw_mode = 1;
	cube_amount_selected = 0;

	if (cube_shader == NULL) {
		cube_shader = new Shader("shaders/asteroids.vs", "shaders/asteroids.fs");
	}
	else {
		std::cout << "using premade shader for the cubes" << std::endl;
	}
	cube = new Model("resources/objects/cube/cube.obj");

	x_width = 20;
	z_width = 20;
	y_width = 0;
	cube_offset = 2.0f;

	terrian_map = new map_tile * [x_width];
	for (int i = 0; i < x_width; i++) {
		terrian_map[i] = new map_tile[z_width];
	}

	/* testing the path finding by setting up a test grid
	1-- > The cell is not blocked
	0-- > The cell is blocked */
	int grid[20][20] = {
		{ 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1 },
		{ 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
	};

	float x = 0;
	float y = 0;
	float z = 0;

	//generate the extra cubes 
	unsigned int amount_extra = 0;

	//finding out the amount of extra cubes
	for (int xi = 0; xi < x_width; xi++) {
		for (int zi = 0; zi < z_width; zi++) {
			if (grid[xi][zi] == 0) {
				amount_extra++;
			}
		}
	}

	amount_extra++;//one more for good mesure
	cube_buffer_size = (x_width * z_width) + amount_extra;
	cube_amount = (x_width * z_width);
	cube_matrices = new glm::mat4[cube_buffer_size];// the main buffer
	cube_matrices_selected = new glm::mat4[cube_buffer_size];// the buffer for the selected cubes
	links = new map_loc[cube_buffer_size];

	bool first = true;

	unsigned int xloc = 0;
	unsigned int zloc = 0;

	//creates the plain of cubes
	for (unsigned int i = 0; i < cube_amount; i++) {
		map_tile temp;
		temp.x = x;
		temp.y = y;
		temp.z = z;
		temp.g_cost = 1;
		temp.blocked = false;
		temp.buffer_loc = i;
		temp.type = 1;
		temp.zoned = NULL;
		temp.item_on_top = NULL;
		temp.plant_on_top = NULL;
		terrian_map[xloc][zloc] = temp;
		//create the buffer to map link
		map_loc temp2;
		temp2.x = xloc;
		temp2.z = zloc;
		links[i] = temp2;

		glm::mat4 model = glm::mat4(1.0f);
		if (x == 0 && z == 0) {
			y = 2 * cube_offset;
		}
		else if (x == (cube_offset * (x_width - 1)) && z == 0) {
			y = 2 * cube_offset;
		}
		else {
			y = 0;
		}
		model = glm::translate(model, glm::vec3(x, y, z));
		cube_matrices[i] = model;
		x += cube_offset;
		xloc++;
		if (x == (cube_offset * x_width)) {
			z += cube_offset;
			zloc++;
			x = 0;
			xloc = 0;
		}

	}

	x = 0;
	y = 0;
	z = 0;
	y += cube_offset;

	vector<glm::mat4> additional_cubes;
	for (int xi = 0; xi < x_width; xi++) {
		for (int zi = 0; zi < z_width; zi++) {

			if (grid[xi][zi] == 0) {
				glm::mat4 blocked_cube = glm::mat4(1.0f);
				//the coridents need to be inverted
				z = xi * cube_offset;
				x = zi * cube_offset;
				terrian_map[xi][zi].blocked = true;
				blocked_cube = glm::translate(blocked_cube, glm::vec3(x, y, z));
				blocked_cube = glm::rotate(blocked_cube, 3.14f, glm::vec3(1.0, 0.0, 0.0));
				additional_cubes.push_back(blocked_cube);
			}
		}
	}

	std::cout << "creating " << cube_amount << " of total " << cube_buffer_size << " spots" << std::endl;
	std::cout << "need to add " << additional_cubes.size() << " more cubes" << std::endl;

	for (size_t i = 0; i < additional_cubes.size(); i++)
	{
		cube_matrices[cube_amount] = additional_cubes[i];
		cube_amount++;
	}

	//print_map_blocked();
	//add in the items
	import_items();
	//test out the map zoning 
	int x1 = 14;
	int y1 = 0;
	int z1 = 14;

	int x2 = 17;
	int y2 = 0;
	int z2 = 17;
	alter_zone = zone_land(ALTER,1, x1, y1, z1, x2, y2, z2);

	
	x1 = 0;
	y1 = 0;
	z1 = int(get_z_width()) - 5;

	x2 = 4;
	y2 = 0;
	z2 = int(get_z_width());
	spawn_zone = zone_land(SPAWN,3, x1, y1, z1, x2, y2, z2);
	//spawn_zone = zone_land(SPAWN, 3, x2, y2, z2, x1, y1, z1);

	x1 = int(get_x_width()-5);
	y1 = 0;
	z1 = 0;

	x2 = int(get_x_width());;
	y2 = 0;
	z2 = 2;
	stockpile_zone_old = zone_land(STOCKPILE,4, x1, y1, z1, x2, y2, z2);

	x1 = int(get_x_width() - 5);
	y1 = 0;
	z1 = 5;
	std::cout << "before the new farm info " << std::endl;

	x2 = int(get_x_width());;
	y2 = 0;
	z2 = 7;
	farm_zone = zone_land(FARM, 5, x1, y1, z1, x2, y2, z2);

	items_to_add = farm_zone->get_grown_items();
	farm_tiles_need_work = farm_zone->get_farm_tiles_need_work();
	std::cout << "after the new farm info "<< std::endl;

	stock_obj = new stockpile_zone;
	stock_obj->zones = stockpile_zone_old;
	stock_obj->used_spots = 0;
	stock_obj->max_spots = stockpile_zone_old->get_max_spots();

	//print_map_zoned();
	//print_map_blocked_zones();
	//while (true);
	//block off the land from the objects that take up space
	std::vector< block_loc*> *blocked_loc = OBJM->get_blocked_spots();
	std::cout << "amount of blocked spaces from objects = " << blocked_loc->size() << std::endl;
	for (int i = 0; i < blocked_loc->size(); i++) {
		block_loc* loc_temp = (*blocked_loc)[i];

		std::cout << "x = " << loc_temp->x << " z = " << loc_temp->z << std::endl;
		block_spot(loc_temp->x, loc_temp->z);
		delete loc_temp;
	}
	blocked_loc->clear();
	//print_map_blocked();

	glGenBuffers(1, &buffer_slected);
	glBindBuffer(GL_ARRAY_BUFFER, buffer_slected);
	glBufferData(GL_ARRAY_BUFFER, cube_amount * sizeof(glm::mat4), &cube_matrices_selected[0], GL_STATIC_DRAW);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, cube_amount * sizeof(glm::mat4), &cube_matrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < cube->meshes.size(); i++)
	{
		unsigned int VAO = cube->meshes[i].VAO;
		glBindVertexArray(VAO);
		// set attribute pointers for matrix (4 times vec4)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}
	

	std::cout << "finished creating" << std::endl;
}

void terrian::select(unsigned char PixelColor[3]) {
	bool color1 = false;
	bool color2 = false;
	bool color3 = false;
	int offset = 0;
	//0 - 254 range for each number
	int buffer_loc = int(PixelColor[0]);

	if (int(PixelColor[1]) != 0) {
		color2 = true;
		offset = 255;
	}
	if (int(PixelColor[2]) != 0) {
		color2 = true;
	}
	//gl_InstanceID%255;
	//float(id)/colors_amount
	buffer_loc += offset;
	std::cout << "cube located at " << buffer_loc<< std::endl;
	
	std::cout << "loc x = " << links[buffer_loc].x << ", loc z = " << links[buffer_loc].z << std::endl;
}

void terrian::space_init() {
	std::cout << "creating the terrian class (space)" << std::endl;
	if (Time != NULL) {
		deltatime = Time->get_time_change();
	}
	else {
		std::cout << "there was a problem getting time in the sky" << std::endl;
		while (true);
	}
    draw_mode = 0;
	asteroidShader = new Shader("shaders/asteroids.vs", "shaders/asteroids.fs");
	planetShader = new Shader("shaders/planet.vs", "shaders/planet.fs");

	rock = new Model("resources/objects/rock/rock.obj");
	planet = new Model("resources/objects/planet/planet.obj");

    unsigned int amount = 100000;
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(glfwGetTime()); // initialize random seed	
    float radius = 150.0;
    float offset = 25.0f;
    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }


    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);


    for (unsigned int i = 0; i < rock->meshes.size(); i++)
    {
        unsigned int VAO = rock->meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }


	std::cout << "finished creating" << std::endl;
}

void terrian::print_map() {
    if (terrian_map != NULL) {
        std::cout << "printing out internal terrian_map representation" << std::endl;
        for (int x = 0; x < x_width; x++) {
            for (int z = 0; z < z_width; z++) {
                switch (terrian_map[x][z].type)
                {
                case 1:
                    std::cout << " * ";
                    break;
                case 2:
                    std::cout << " "<< terrian_map[x][z].buffer_loc;
                    break;
                default:
                    std::cout <<x<<" "<<z<<" not defined proporly"<< std::endl;
                    break;
                }
            }
            std::cout << std::endl;
        }
    }
    else {
        std::cout << "the terrian_map data structure was never created" << std::endl;
    }

}

void terrian::print_map_blocked() {
    if (terrian_map != NULL) {
        std::cout << "printing out internal terrian_map representation" << std::endl;
        for (int x = 0; x < x_width; x++) {
            for (int z = 0; z < z_width; z++) {
                if (!terrian_map[x][z].blocked) {
                    std::cout << "1 ";
                }
                else {
                    std::cout << ". ";
                }
            }
            std::cout << std::endl;
        }
    }
    else {
        std::cout << "the terrian_map data structure was never created" << std::endl;
    }
}

void terrian::block_spot(int x_loc, int z_loc) {
	terrian_map[x_loc][z_loc].blocked = true;
	//std::cout << "blocking map spot" << std::endl;
}

glm::vec3 terrian::get_coridents(int x_loc, int z_loc) {
	return glm::vec3(x_loc * cube_offset,cube_offset, z_loc * cube_offset);
}

//prints out the zoned map
void terrian::print_map_zoned() {
	if (terrian_map != NULL) {
		std::cout << "printing out internal terrian_map representation" << std::endl;
		for (int x = 0; x < x_width; x++) {
			for (int z = 0; z < z_width; z++) {
				if (terrian_map[x][z].zoned != NULL) {
					switch (terrian_map[x][z].zoned->get_type()) {
					case SPAWN:
						std::cout << "1 ";
						break;
					case ALTER:
						std::cout << "2 ";
						break;
					case GATHER:
						std::cout << "3 ";
						break;
					case STOCKPILE:
						std::cout << "4 ";
						break;
					case FARM:
						std::cout << "5 ";
						break;
					case RELAXATION:
						std::cout << "6 ";
						break;
					case MEETING_Z:
						std::cout << "7 ";
						break;
					default:
						std::cout << "* ";
						break;
					}
				}
				else {
					std::cout << "0 ";
				}
			}
			std::cout << std::endl;
		}
	}
	else {
		std::cout << "the terrian_map data structure was never created" << std::endl;
	}
}

//zone a grid of spaces
zone* terrian::zone_land(type tp, unsigned int id, int x1, int y1, int z1, int x2, int y2, int z2) {
	zone* fresh_zone = new zone(tp, id);
	bool blocked;
	std::cout << "zoning land" << std::endl;
	if (x1 == x2 && y1 == y2 && z1 == z2) {
		std::cout << "single space" << std::endl;
		blocked = terrian_map[z1][x1].blocked;
		fresh_zone->add_spot(x1, y1, z1, blocked);
		terrian_map[z1][x1].zoned = fresh_zone;
	}
	else {
		std::cout << "big space" << std::endl;
		int width;
		int height;
		int s_x;//start pos
		int s_z;

		if (x1 > x2) {
			s_x = x2;
			width = x1 - x2;
		}
		else {
			s_x = x1;
			width = x2 - x1;
		}

		if (z1 > z2) {
			s_z = y2;
			height = z1 - z2;
		}
		else {
			s_z = z1;
			height = z2 - z1;
		}
		std::cout << "start: x = " << s_x << ", y = " << s_z << std::endl;
		std::cout << "width = " << width << ", height = " << height << std::endl;
		for (int x = 0; x < width; x++) {
			for (int z = 0; z < height; z++) {
				terrian_map[s_z + z][s_x + x].zoned = fresh_zone;
				blocked = terrian_map[s_z + z][s_x + x].blocked;
				fresh_zone->add_spot(s_x + x, y1, s_z + z, blocked);
			}
		}
		//fresh_zone->print_info();
		
	}
	std::cout << "done zoning land" << std::endl;
	return fresh_zone;
}

void terrian::print_map_blocked_zones() {

		if (terrian_map != NULL) {
			std::cout << "printing out internal terrian_map representation of zones and blocked points" << std::endl;
			for (int x = 0; x < x_width; x++) {
				for (int z = 0; z < z_width; z++) {
					if (terrian_map[x][z].zoned != NULL) {
						if (terrian_map[x][z].blocked) {
							std::cout << "* ";
						}
						else {
							switch (terrian_map[x][z].zoned->get_type()) {
							case SPAWN:
								std::cout << "1 ";
								break;
							case ALTER:
								std::cout << "2 ";
								break;
							case GATHER:
								std::cout << "3 ";
								break;
							case STOCKPILE:
								std::cout << "4 ";
								break;
							case FARM:
								std::cout << "5 ";
								break;
							case RELAXATION:
								std::cout << "6 ";
								break;
							case MEETING_Z:
								std::cout << "7 ";
								break;
							default:
								std::cout << ". ";
								break;
							}
						}
					}
					else {
						std::cout << "0 ";
					}
				}
				std::cout << std::endl;
			}
		}
		else {
			std::cout << "the terrian_map data structure was never created" << std::endl;
		}
}

void terrian::harvest_farm_tile(farm_tile* tile) {
	
	//item_info* temp = spawn_item(FRUIT_T, tile->loc->x, tile->loc->z);
	//then generate a work order for the object
	if (terrian_map[tile->loc->z][tile->loc->x].item_on_top != NULL) {
		gen_orders->push_back(generate_work_order(SACRIFICE_OBJ, tile->loc->x, 5, tile->loc->z));
	}
	else {
		//std::cout << "no a plant product to harvest" << std::endl;
	}
}

void terrian::plant(farm_tile* tile) {
	std::cout << "planting a plant" << std::endl;
	item_info* temp = spawn_item(FRUIT_PLANT, tile->loc->x, tile->loc->z);
	item_info* temp2 = spawn_item(FRUIT_T, tile->loc->x, tile->loc->z);
	temp->grown_item = temp2;
	temp->farm_t = tile;
	terrian_map[tile->loc->z][tile->loc->x].plant_on_top = temp;
	//print_map_plants();
}

void terrian::print_map_plants() {

	if (terrian_map != NULL) {
		std::cout << "printing out internal terrian_map representation of zones and blocked points" << std::endl;
		for (int x = 0; x < x_width; x++) {
			for (int z = 0; z < z_width; z++) {
				if (terrian_map[x][z].plant_on_top != NULL){ 
						std::cout << "* ";
				}
				else {
					std::cout << "0 ";
				}
			}
			std::cout << std::endl;
		}
	}
	else {
		std::cout << "the terrian_map data structure was never created" << std::endl;
	}
}

//task creation function
void terrian::print_work_order(work_order* wo) {
	std::cout << "printing a work order" << std::endl;
	if (wo != NULL) {
		std::string obj;
		std::string action;
		std::string overall_job;

		switch (wo->job) {
		case STOCK_OBJ:
			overall_job = "STOCK_OBJ";
			break;
		case SACRIFICE_OBJ:
			overall_job = "SACRIFICE_OBJ";
			break;
		case START_SACRIFICE:
			overall_job = "START_SACRIFICE";
			break;
		case MOVE_C:
			overall_job = "MOVE_C";
			break;
		default:
			overall_job = "not on list of approved jobs";
			break;
		}

		std::cout << "overall job = " << overall_job << std::endl;
		if (wo->object != NULL) {
			//obj = *(wo->object->item_name);
		}
		else {

		}
		obj = "NULL";
		std::cout << "action currently on " << wo->currently_on << std::endl;
		std::cout << "total action " << wo->action_numbers << std::endl;
		for (int i = 0; i < wo->action_numbers; i++) {
			switch (wo->action_rq[i]) {
			case PICK_UP:
				std::cout << "action required: PICK_UP" << std::endl;
				break;
			case DROP:
				std::cout << "action required: DROP" << std::endl;
				break;
			case SAC_OBJ:
				std::cout << "action required: SAC_OBJ" << std::endl;
				break;
			case MOVE:
				std::cout << "action required: MOVE" << std::endl;
				break;
			default:
				std::cout << "not on list of approved actions" << std::endl;
				break;
			}
		}

		std::cout << "object needed = " << obj << std::endl;

		if (wo->destination != NULL) {
			float x = wo->destination->x;
			float z = wo->destination->z;
			printf("destination = %f , %f \n", x, z);
		}
		else {
			std::cout << "destination  = NULL" << std::endl;
		}
		
		std::cout << "has arrived: " << wo->arrived << std::endl;
	}
	else {
		std::cout << "not a work order" << std::endl;
	}
}

//TODO: fix function to get random location to move to
int* terrian::get_random_map_loc(int s_x, int s_y, int s_z, int dist) {
	int* output = new int[3];
	output[0] = s_x;
	output[1] = s_y;
	output[2] = s_z;
	std::random_device rd;
	int seed = rd();
	std::mt19937 mt(seed);
	std::uniform_real_distribution<double> distribution(0, 6);
	int selection = distribution(mt);
	int start = selection;
	bool unblocked = false;

	switch (selection) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
	default:

		break;
	}
	return output;
}

void terrian::print_currently_on(work_order* wo) {
	std::cout << "action: ";
	switch (wo->action_rq[wo->currently_on]) {
	case PICK_UP:
		  std::cout << "picking up item" << std::endl;
		
		break;
	case DROP:
		std::cout << "dropping item" << std::endl;

		break;
	case SAC_OBJ:
		 std::cout << "sacrificing item" << std::endl;

		break;
	case START_SAC:
		std::cout << "starting sacrifice" << std::endl;

		break;
	case TILL:
		 std::cout << "tilling soil" << std::endl;

		break;
	case TEND:
		 std::cout << "tending plant" << std::endl;

		break;
	case HARVEST:
		 std::cout << "harvesting plant" << std::endl;

		break;
	default:
		 std::cout << "unkown action" << std::endl;
		break;
	}

}

bool terrian::action_requires_time(work_order* wo) {
	switch (wo->action_rq[wo->currently_on]) {
	case SAC_OBJ:
	case START_SAC:
	case TILL:
	case TEND:
	case HARVEST:
	case RELAXING:
		return true;
	}
	return false;
}

work_order* terrian::generate_work_order(work_jobs work_job, int x1, int y1, int z1, farm_tile* f_tile) {
	//std::cout << "creating work order" << std::endl;
	work_order* temp;

	//generates one work order

	//std::cout << "single space" << std::endl;
	temp = new work_order;
	temp->job = work_job;//the overall job
	temp->act_currently_on = 0;
	temp->loc_currently_on = 0;
	temp->currently_on = 0;
	item_info* obj_dest;
	temp->arrived = false;
	temp->zone_location = NULL;
	//std::cout << "checking " << x1 << " " << z1 << std::endl;
	if (terrian_map[z1][x1].item_on_top != NULL) {
		//	std::cout << "1 item on top " << std::endl;
		obj_dest = terrian_map[z1][x1].item_on_top;
	}
	else {
		//std::cout << "0 item on top" << std::endl;
		obj_dest = NULL;
	}
	temp->time_length = 0;
	temp->time_spent = 0;
	temp->object = obj_dest;
	unsigned int action_numbers;
	unsigned int location_amount;
	zone_loc* store;

	temp->farm_t = f_tile;//should def to NULL 

	switch (work_job) {
	case STOCK_OBJ:
		action_numbers = 2;
		location_amount = 2;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->action_rq = new action[action_numbers];
		//temp->object = OBJM->get_item_info();//just as a temp thing untill th object handler gets updated
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		store = stockpile_zone_old->get_stockpile_loc();
		store->origin = stockpile_zone_old;
		temp->zone_location = store;
		temp->destination[1].x = store->x;
		temp->destination[1].y = store->y;
		temp->destination[1].z = store->z;
		temp->action_rq[0] = PICK_UP;
		//temp->action_rq[1] = MOVE;
		temp->action_rq[1] = DROP;
		temp->job_t = DUMB;
		break;
	case SACRIFICE_OBJ:
		action_numbers = 2;
		location_amount = 2;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		//temp->object = OBJM->get_item_info();//just as a temp thing untill th object handler gets updated
		temp->destination = new map_loc[location_amount];
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		store = alter_zone->get_alter_loc();
		store->origin = alter_zone;
		temp->zone_location = store;
		temp->destination[1].x = store->x;
		temp->destination[1].y = store->y;
		temp->destination[1].z = store->z;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = PICK_UP;
		//temp->action_rq[1] = MOVE;
		temp->action_rq[1] = SAC_OBJ;
		temp->job_t = RELIGION;
		temp->time_length = Time->get_sacrifice_time();
		break;
	case START_SACRIFICE://if there is queue of things to be sacrificed then this work order will be created
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = START_SAC;
		temp->job_t = RELIGION;
		break;
	case WANDER:
	case RELAX:
		//std::cout << "creating a relax/wander order" << std::endl;
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		store = meeting_zone->get_meeting_loc();
	/*	if (store == NULL) {
			std::cout << "found error" << std::endl;
			while (true);
		}*/
		temp->destination->x = store->x;
		temp->destination->y = store->y;
		temp->destination->z = store->z;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = RELAXING;
		temp->job_t = NONE;
		temp->time_length = Time->get_relax_time();
		break;
	case MOVE_C:
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->destination->x = x1;
		temp->destination->y = y1;
		temp->destination->z = z1;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = MOVE;
		temp->job_t = NONE;
		break;
	case  TILL_SOIL:
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = TILL;
		temp->job_t = AGRICULTURE;
		temp->time_length = Time->get_till_soil_time();
		break;
	case TEND_PLANT:
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = TEND;
		temp->job_t = AGRICULTURE;
		temp->time_length = Time->get_tend_plant_time();
		break;
	case HARVEST_PLANT:
		action_numbers = 1;
		location_amount = 1;
		temp->action_numbers = action_numbers;
		temp->location_amount = location_amount;
		temp->destination = new map_loc[location_amount];
		temp->destination[0].x = x1;
		temp->destination[0].y = y1;
		temp->destination[0].z = z1;
		temp->action_rq = new action[action_numbers];
		temp->action_rq[0] = HARVEST;
		temp->job_t = AGRICULTURE;
		temp->time_length = Time->get_harvest_plant_time();
		break;
	default:
		break;
	}
	//print_work_order(temp);
	return temp;
}

std::vector<work_order*> terrian::generate_work_order_m(work_jobs work_job, int x1, int y1, int z1, int x2, int y2, int z2) {
	std::vector<work_order*> output;
	//	std::cout << "creating multiple orders" << std::endl;
		//std::cout << "inputs x1: "<<x1<<" z1: "<<z1<<" x2: " <<x2<<" z2: "<< z2 << std::endl;

	if ((x1 == x2 && y1 == y2 && z1 == z2) || (x2 == -1 || y2 == -1 || z2 == -1)) {
		output.push_back(generate_work_order(work_job, x1, y1, z1));
	}
	else {
		int x_s, width;
		int z_s, height;
		if (x1 == x2) {//only the z cords are changing 
			//std::cout << "zline" << std::endl;
			if (z1 > z2) {
				z_s = z2;
				height = z1 - z2;
			}
			else {
				z_s = z1;
				height = z2 - z1;
			}

			for (int i = 0; i < height; i++) {
				//std::cout << z_s + i << " ";
				if (terrian_map[z_s + i][x1].item_on_top != NULL) {
					output.push_back(generate_work_order(work_job, x1, 5, z_s + i));
				}
			}
		}
		else if (z1 == z2) {//only the x cords are changing 
		//	std::cout << "xline" << std::endl;
			if (x1 > x2) {
				x_s = x2;
				width = x1 - x2;
			}
			else {
				x_s = x1;
				width = x2 - x1;
			}

			for (int i = 0; i < width; i++) {
				//std::cout << x_s + i << " ";
				if (terrian_map[z1][x_s + i].item_on_top != NULL) {
					output.push_back(generate_work_order(work_job, x_s + i, 5, z1));
				}
			}
		}
		else {
			//	std::cout << "a box" << std::endl;
			if (z1 > z2) {
				z_s = z2;
				height = z1 - z2;
			}
			else {
				z_s = z1;
				height = z2 - z1;
			}
			if (x1 > x2) {
				x_s = x2;
				width = x1 - x2;
			}
			else {
				x_s = x1;
				width = x2 - x1;
			}

			for (int i = 0; i < width; i++) {
				for (int q = 0; q < height; q++) {
					if (terrian_map[z_s + q][x_s + i].item_on_top != NULL) {
						output.push_back(generate_work_order(work_job, x_s + i, 5, z_s + q));
					}
				}
			}
		}
	}

	return output;
}

void terrian::delete_work_order(work_order* work_job) {
	work_job->object = NULL;
	delete[] work_job->destination;
	delete[] work_job->action_rq;
	delete work_job;
}

void terrian::print_map_items() {

	if (terrian_map != NULL) {
		std::cout << "printing out internal terrian_map representation" << std::endl;
		for (int x = 0; x < x_width; x++) {
			for (int z = 0; z < z_width; z++) {
				if (terrian_map[x][z].item_on_top != NULL) {
					std::cout << terrian_map[x][z].item_on_top->debug_id<<" ";
				}
				else {
					std::cout << "* ";
				}
			}
			std::cout << std::endl;
		}
	}
	else {
		std::cout << "the terrian_map data structure was never created" << std::endl;
	}
}

void terrian::print_map_items_stacks() {

	if (terrian_map != NULL) {
		std::cout << "printing out internal terrian_map representation" << std::endl;
		for (int x = 0; x < x_width; x++) {
			for (int z = 0; z < z_width; z++) {
				if (terrian_map[x][z].item_on_top != NULL) {
					std::cout << terrian_map[x][z].item_on_top->stack_size << " ";
				}
				else {
					std::cout << "* ";
				}
			}
			std::cout << std::endl;
		}
	}
	else {
		std::cout << "the terrian_map data structure was never created" << std::endl;
	}
}

void terrian::import_items() {
	std::vector< item_loc> items_on_map = OBJM->place_items_init();

	if (items_on_map.size() > 0) {
		for (int i = 0; i < items_on_map.size(); i++) {
			terrian_map[items_on_map[i].z][items_on_map[i].x].item_on_top = items_on_map[i].object;
			if (items_on_map[i].object->type == ALTER_T || items_on_map[i].object->type == BED || items_on_map[i].object->type == TABLE) {
				terrian_map[items_on_map[i].z][items_on_map[i].x].blocked = true;
			}
			if (items_on_map[i].object->type == CAMP_FIRE) {
				std::cout << "campfire was placed, creating meeting zone" << std::endl;
				int x = items_on_map[i].x;
				int z = items_on_map[i].z;
				terrian_map[items_on_map[i].z][items_on_map[i].x].blocked = true;
				int m_x1 = x - 2;
				int m_x2 = x + 2;
				int m_z1 = z - 3;
				int m_z2 = z + 3;

				if (m_x1 < 0) {
					m_x1 = 0;
				}
				if (m_z1 < 0) {
					m_z1 = 0;
				}

				if (m_x2 >= x_width) {
					m_x2 = x_width - 1;
				}

				if (m_z2 >= z_width) {
					m_z2 = z_width - 1;
				}

				meeting_zone = zone_land(MEETING_Z, 5, m_x1, 7, m_z1, m_x2, 7, m_z2);
				//print_map_zoned();
				//print_map_blocked_zones();
				//while (true);
			}
		}
	}
	else {
		std::cout << "there was no items to import" << std::endl;
	}

	//print_map_items();
}

void terrian::remove_item_from_map(item_info* i) {
	int x = i->x_m;
	int y = i->y_m;
	int z = i->z_m;
	if (terrian_map[z][x].plant_on_top != NULL) {
		terrian_map[z][x].plant_on_top->grown_item = spawn_item(FRUIT_T, x, z);
	}
	else {
		terrian_map[z][x].item_on_top = NULL;
	}
}
void terrian::add_item_to_map(item_info* i) {
	int x = i->x_m;
	int y = i->y_m;
	int z = i->z_m;

	if (i->zone_location != NULL && i->zone_location->origin->get_type() == STOCKPILE) {
		std::cout << "adding item to stockpile" << std::endl;
		add_item_to_stock_pile(i);
	}
	else {
		terrian_map[z][x].item_on_top = i;
	}

	//print_map_items_stacks();
}

void terrian::add_item_to_stock_pile(item_info* i) {
	//print_stockpile_zone();
	int x = i->x_m;
	int y = i->y_m;
	int z = i->z_m;

	if (stock_obj->partly_spots.empty()) {
		std::cout << "no partly filled spots" << std::endl;

		stock_zone_info* temp = new stock_zone_info;
		temp->stored_item = i;
		temp->loc = i->zone_location;
		temp->zone_loc_ID= i->zone_location->ID;
		temp->stack_number;
		temp->max_stack = i->stack_size;
		temp->origin = stock_obj;
		if (i->max_stack()) {
			stock_obj->filled_spots.push_back(temp);
		}
		else {
			stock_obj->partly_spots.push_back(temp);
		}
		stock_obj->used_spots++;
		terrian_map[z][x].item_on_top = i;
	}
	else {
		std::cout << "there are partly filled spots" << std::endl;
		bool found = false;
		for (int q = 0; q < stock_obj->partly_spots.size(); q++) {
			if (i->type == stock_obj->partly_spots[q]->stored_item->type) {
				std::cout << "found matching type" << std::endl;
				found = true;
				int total = stock_obj->partly_spots[q]->stored_item->stack_size + i->stack_size;
				if (stock_obj->partly_spots[q]->stored_item->max_stack_size >= total) {
					stock_obj->partly_spots[q]->stored_item->stack_size += i->stack_size;
					OBJM->delete_item_from_buffer(i);//destories the item so it is not drawn anymore 
					break;
				}
				else {
					int diff = total - stock_obj->partly_spots[q]->stored_item->max_stack_size;
					//move the old pile to the filled_spots
					stock_obj->partly_spots[q]->stored_item->stack_size = stock_obj->partly_spots[q]->stored_item->max_stack_size;
					stock_obj->partly_spots[q]->stack_number = stock_obj->partly_spots[q]->max_stack;
					stock_zone_info* temp_z = stock_obj->partly_spots[q];
					stock_obj->partly_spots[q] = stock_obj->partly_spots[0];
					stock_obj->partly_spots.erase(stock_obj->partly_spots.begin());
					stock_obj->filled_spots.push_back(temp_z);

					//creat the new pile
					i->stack_size = diff;
					stock_zone_info* temp = new stock_zone_info;
					temp->stored_item = i;
					temp->loc = i->zone_location;
					temp->zone_loc_ID = i->zone_location->ID;
					temp->stack_number;
					temp->max_stack = i->stack_size;
					temp->origin = stock_obj;
					if (i->max_stack()) {
						stock_obj->filled_spots.push_back(temp);
					}
					else {
						stock_obj->partly_spots.push_back(temp);
					}
					stock_obj->used_spots++;
					terrian_map[z][x].item_on_top = i;

					terrian_map[z][x].item_on_top = i;
					break;
				}
			}
		}
		if (!found) {
			std::cout << "there are no partly filled spots for this item type" << std::endl;
			stock_zone_info* temp = new stock_zone_info;
			temp->stored_item = i;
			temp->loc = i->zone_location;
			temp->zone_loc_ID = i->zone_location->ID;
			temp->stack_number;
			temp->max_stack = i->stack_size;
			temp->origin = stock_obj;
			if (i->max_stack()) {
				stock_obj->filled_spots.push_back(temp);
			}
			else {
				stock_obj->partly_spots.push_back(temp);
			}
			stock_obj->used_spots++;
			terrian_map[z][x].item_on_top = i;
		}
	}

	print_stockpile_zone();
}

void terrian::remove_item_to_stock_pile(item_info* i) {

}

void terrian::print_stockpile_zone() {
	std::cout << "sotckpile zone info" << std::endl;
	std::cout << "max spots " << stock_obj->max_spots << std::endl;
	std::cout << "used spots " << stock_obj->used_spots << std::endl;
	std::cout << "filled spots (vector) " << stock_obj->filled_spots.size() << std::endl;
	std::cout << "partly spots (vector) " << stock_obj->partly_spots.size() << std::endl;
}

void terrian::add_item_to_alter(item_info* i) {
	OBJM->preform_sacrifice(i);
}

void terrian::return_zone_loc(zone_loc* i) {
	switch (i->ID){
	case 1://alter
		alter_zone->remove_item_from_spot(i);
		break;
	//case 2://gather
	//	break;
	case 3://stock
		stockpile_zone_old->remove_item_from_spot(i);
		break;
	//case 4://spawn
	//	stockpile_zone_old->remove_item_from_spot(i);
	//	break;
	default:
		std::cout << "no zone with this ID" << std::endl;
		break;
	}

}

item_info* terrian::spawn_item(item_type type, int x, int z) {
	item_info* output = OBJM->spawn_item(type,x,z);
	terrian_map[z][x].item_on_top = output;
	//print_map_items();

	if (type == CAMP_FIRE) {
		std::cout << "campfire was placed, creating meeting zone" << std::endl;

		terrian_map[z][x].blocked = true;
		int m_x1 = x - 2;
		int m_x2 = x + 2;
		int m_z1 = z - 3;
		int m_z2 = z + 3;

		if (m_x1 < 0) {
			m_x1 = 0;
		}
		if (m_z1 < 0) {
			m_z1 = 0;
		}

		if (m_x2 >= x_width) {
			m_x2 = x_width - 1;
		}

		if (m_z2 >= z_width) {
			m_z2 = z_width - 1;
		}

		meeting_zone = zone_land(MEETING_Z, 5, m_x1, 7, m_z1, m_x2, 7, m_z2);
	}
	return output;
}

//path finding functions

//makesure to swap the input of x and z due to the implementtion of the function
std::vector<glm::vec3*> terrian::find_path(int x1, int z1, int x2,int z2, float height) {

    vector<glm::vec3*> output;

	Pair src = make_pair(x1, z1);
	Pair dest = make_pair(x2, z2);

	cell** cellDetails = aStarSearch(src, dest);

	//printf("\nThe Path is ");
	int row = dest.first;
	int col = dest.second;

	stack<Pair> Path;
	if (cellDetails != NULL) {
		while (!(cellDetails[row][col].parent_i == row
			&& cellDetails[row][col].parent_j == col))
		{
			Path.push(make_pair(row, col));
			int temp_row = cellDetails[row][col].parent_i;
			int temp_col = cellDetails[row][col].parent_j;
			row = temp_row;
			col = temp_col;
		}

		glm::vec3* temp;
		Path.push(make_pair(row, col));
		while (!Path.empty())
		{
			pair<int, int> p = Path.top();
			Path.pop();
			float x = p.first * cube_offset;
			float z = p.second * cube_offset;
			//printf("-> (%f,%f) ",x, z);
			temp = new glm::vec3(z, height, x);
			output.push_back(temp);
		}

		//std::cout << "current path" << std::endl;
		//for (int i = 0; i < output.size(); i++) {
		//	std::cout << "x: " << output[i]->x << " y: " << output[i]->y << " z: " << output[i]->z << std::endl;
		//}

		//clean used mem
		for (int i = 0; i < x_width; i++) {
			delete[]  cellDetails[i];
		}
		delete[]  cellDetails;
	}
	else {
		//std::cout << "cellDetails was NULL" << std::endl;
		if (x1 == x2 && z1 == z2) {//already at the same destination
			output.push_back(new glm::vec3(z1 * cube_offset, height, x1 * cube_offset));
		}
		else {
			output.push_back(new glm::vec3(-4, -4, -4));// placed so the program does not crash
		}
	}
    return output;
}

bool terrian::isValid(int row, int col)
{
	// Returns true if row number and column number 
	// is in range 
	return (row >= 0) && (row < x_width) &&
		(col >= 0) && (col < z_width);
}

bool terrian::isUnBlocked(int row, int col)
{
	return !terrian_map[row][col].blocked;
}

// A Utility Function to check whether destination cell has 
// been reached or not 
bool terrian::isDestination(int row, int col, Pair dest)
{
	if (row == dest.first && col == dest.second)
		return (true);
	else
		return (false);
}

// A Utility Function to calculate the 'h' heuristics. 
double terrian::calculateHValue(int row, int col, Pair dest)
{
	// Return using the distance formula 
	return ((double)sqrt((row - dest.first) * (row - dest.first)
		+ (col - dest.second) * (col - dest.second)));
}

// A Utility Function to trace the path from the source 
// to destination 
void terrian::tracePath(cell** cellDetails, Pair dest)
{
	printf("\nThe Path is ");
	int row = dest.first;
	int col = dest.second;

	stack<Pair> Path;

	while (!(cellDetails[row][col].parent_i == row
		&& cellDetails[row][col].parent_j == col))
	{
		Path.push(make_pair(row, col));
		int temp_row = cellDetails[row][col].parent_i;
		int temp_col = cellDetails[row][col].parent_j;
		row = temp_row;
		col = temp_col;
	}

	Path.push(make_pair(row, col));
	while (!Path.empty())
	{
		pair<int, int> p = Path.top();
		Path.pop();
		printf("-> (%d,%d) ", p.first, p.second);
	}

	return;
}


cell** terrian::aStarSearch(Pair src, Pair dest)
{

	if (isValid(src.first, src.second) == false)
	{
		printf("Source is invalid\n");
		return NULL;
	}


	if (isValid(dest.first, dest.second) == false)
	{
		printf("Destination is invalid\n");
		return NULL;
	}


	if (isUnBlocked(src.first, src.second) == false ||
		isUnBlocked(dest.first, dest.second) == false)
	{
		printf("Source or the destination is blocked\n");
		return NULL;
	}


	if (isDestination(src.first, src.second, dest) == true)
	{
		//printf("We are already at the destination\n");
		return NULL;
	}



	// Create a closed list and initialise it to false which means 
	// that no cell has been included yet 

	if (closedList == NULL) {
		std::cout << "creating closedList for path fining" << std::endl;
		closedList = new bool* [x_width];
		for (int i = 0; i < x_width; i++) {
			closedList[i] = new bool[z_width];
		}
	}

	for (int x = 0; x < x_width; x++) {
		for (int z = 0; z < z_width; z++) {
			closedList[x][z] = false;
		}
	}

	// Declare a 2D array of structure to hold the details 
	//of that cell 
	cell** cellDetails = new cell * [x_width];
	for (int i = 0; i < x_width; i++) {
		cellDetails[i] = new cell[z_width];
	}


	int i, j;

	for (i = 0; i < x_width; i++)
	{
		for (j = 0; j < z_width; j++)
		{
			cellDetails[i][j].f = FLT_MAX;
			cellDetails[i][j].g = FLT_MAX;
			cellDetails[i][j].h = FLT_MAX;
			cellDetails[i][j].parent_i = -1;
			cellDetails[i][j].parent_j = -1;
		}
	}

	// Initialising the parameters of the starting node 
	i = src.first, j = src.second;
	cellDetails[i][j].f = 0.0;
	cellDetails[i][j].g = 0.0;
	cellDetails[i][j].h = 0.0;
	cellDetails[i][j].parent_i = i;
	cellDetails[i][j].parent_j = j;

	/*
	Create an open list having information as-
	<f, <i, j>>
	where f = g + h,
	and i, j are the row and column index of that cell
	Note that 0 <= i <= ROW-1 & 0 <= j <= COL-1
	This open list is implenented as a set of pair of pair.*/
	set<pPair> openList;

	// Put the starting cell on the open list and set its 
	// 'f' as 0 
	openList.insert(make_pair(0.0, make_pair(i, j)));

	// We set this boolean value as false as initially 
	// the destination is not reached. 
	bool foundDest = false;

	while (!openList.empty())
	{
		pPair p = *openList.begin();

		// Remove this vertex from the open list 
		openList.erase(openList.begin());

		// Add this vertex to the closed list 
		i = p.second.first;
		j = p.second.second;
		closedList[i][j] = true;

		/*
			Generating all the 8 successor of this cell

				N.W N N.E
				\ | /
				\ | /
				W----Cell----E
					/ | \
				/ | \
				S.W S S.E

			Cell-->Popped Cell (i, j)
			N --> North	 (i-1, j)
			S --> South	 (i+1, j)
			E --> East	 (i, j+1)
			W --> West		 (i, j-1)
			N.E--> North-East (i-1, j+1)
			N.W--> North-West (i-1, j-1)
			S.E--> South-East (i+1, j+1)
			S.W--> South-West (i+1, j-1)*/

			// If it isn�t on the open list, add it to 
			// the open list. Make the current square 
			// the parent of this square. Record the 
			// f, g, and h costs of the square cell 
			//			 OR 
			// If it is on the open list already, check 
			// to see if this path to that square is better, 
			// using 'f' cost as the measure. 

					// To store the 'g', 'h' and 'f' of the 8 successors 
		double gNew, hNew, fNew;

		//----------- 1st Successor (North) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i - 1, j) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i - 1, j, dest) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i - 1][j].parent_i = i;
				cellDetails[i - 1][j].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}
			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i - 1][j] == false &&
				isUnBlocked(i - 1, j) == true)
			{
				gNew = cellDetails[i][j].g + 1.0;
				hNew = calculateHValue(i - 1, j, dest);
				fNew = gNew + hNew;


				if (cellDetails[i - 1][j].f == FLT_MAX ||
					cellDetails[i - 1][j].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i - 1, j)));

					// Update the details of this cell 
					cellDetails[i - 1][j].f = fNew;
					cellDetails[i - 1][j].g = gNew;
					cellDetails[i - 1][j].h = hNew;
					cellDetails[i - 1][j].parent_i = i;
					cellDetails[i - 1][j].parent_j = j;
				}
			}
		}

		//----------- 2nd Successor (South) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i + 1, j) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i + 1, j, dest) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i + 1][j].parent_i = i;
				cellDetails[i + 1][j].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}
			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i + 1][j] == false &&
				isUnBlocked(i + 1, j) == true)
			{
				gNew = cellDetails[i][j].g + 1.0;
				hNew = calculateHValue(i + 1, j, dest);
				fNew = gNew + hNew;


				if (cellDetails[i + 1][j].f == FLT_MAX ||
					cellDetails[i + 1][j].f > fNew)
				{
					openList.insert(make_pair(fNew, make_pair(i + 1, j)));
					// Update the details of this cell 
					cellDetails[i + 1][j].f = fNew;
					cellDetails[i + 1][j].g = gNew;
					cellDetails[i + 1][j].h = hNew;
					cellDetails[i + 1][j].parent_i = i;
					cellDetails[i + 1][j].parent_j = j;
				}
			}
		}

		//----------- 3rd Successor (East) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i, j + 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i, j + 1, dest) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i][j + 1].parent_i = i;
				cellDetails[i][j + 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i][j + 1] == false &&
				isUnBlocked(i, j + 1) == true)
			{
				gNew = cellDetails[i][j].g + 1.0;
				hNew = calculateHValue(i, j + 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i][j + 1].f == FLT_MAX ||
					cellDetails[i][j + 1].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i, j + 1)));

					// Update the details of this cell 
					cellDetails[i][j + 1].f = fNew;
					cellDetails[i][j + 1].g = gNew;
					cellDetails[i][j + 1].h = hNew;
					cellDetails[i][j + 1].parent_i = i;
					cellDetails[i][j + 1].parent_j = j;
				}
			}
		}

		//----------- 4th Successor (West) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i, j - 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i, j - 1, dest) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i][j - 1].parent_i = i;
				cellDetails[i][j - 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i][j - 1] == false &&
				isUnBlocked(i, j - 1) == true)
			{
				gNew = cellDetails[i][j].g + 1.0;
				hNew = calculateHValue(i, j - 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i][j - 1].f == FLT_MAX ||
					cellDetails[i][j - 1].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i, j - 1)));

					// Update the details of this cell 
					cellDetails[i][j - 1].f = fNew;
					cellDetails[i][j - 1].g = gNew;
					cellDetails[i][j - 1].h = hNew;
					cellDetails[i][j - 1].parent_i = i;
					cellDetails[i][j - 1].parent_j = j;
				}
			}
		}

		//----------- 5th Successor (North-East) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i - 1, j + 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i - 1, j + 1, dest) == true && isUnBlocked(i, j + 1) == true && isUnBlocked(i - 1, j) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i - 1][j + 1].parent_i = i;
				cellDetails[i - 1][j + 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i - 1][j + 1] == false &&
				(isUnBlocked(i - 1, j + 1) && isUnBlocked(i, j + 1) == true && isUnBlocked(i - 1, j) == true))//prevent cornner cutting
			{
				gNew = cellDetails[i][j].g + 1.414;
				hNew = calculateHValue(i - 1, j + 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i - 1][j + 1].f == FLT_MAX ||
					cellDetails[i - 1][j + 1].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i - 1, j + 1)));

					// Update the details of this cell 
					cellDetails[i - 1][j + 1].f = fNew;
					cellDetails[i - 1][j + 1].g = gNew;
					cellDetails[i - 1][j + 1].h = hNew;
					cellDetails[i - 1][j + 1].parent_i = i;
					cellDetails[i - 1][j + 1].parent_j = j;
				}
			}
		}

		//----------- 6th Successor (North-West) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i - 1, j - 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i - 1, j - 1, dest) == true && isUnBlocked(i, j - 1) == true && isUnBlocked(i - 1, j) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i - 1][j - 1].parent_i = i;
				cellDetails[i - 1][j - 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i - 1][j - 1] == false &&
				(isUnBlocked(i - 1, j - 1) == true && isUnBlocked(i, j - 1) == true && isUnBlocked(i - 1, j) == true))
			{
				gNew = cellDetails[i][j].g + 1.414;
				hNew = calculateHValue(i - 1, j - 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i - 1][j - 1].f == FLT_MAX ||
					cellDetails[i - 1][j - 1].f > fNew)
				{
					openList.insert(make_pair(fNew, make_pair(i - 1, j - 1)));
					// Update the details of this cell 
					cellDetails[i - 1][j - 1].f = fNew;
					cellDetails[i - 1][j - 1].g = gNew;
					cellDetails[i - 1][j - 1].h = hNew;
					cellDetails[i - 1][j - 1].parent_i = i;
					cellDetails[i - 1][j - 1].parent_j = j;
				}
			}
		}

		//----------- 7th Successor (South-East) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i + 1, j + 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i + 1, j + 1, dest) == true && isUnBlocked(i, j + 1) == true && isUnBlocked(i + 1, j) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i + 1][j + 1].parent_i = i;
				cellDetails[i + 1][j + 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i + 1][j + 1] == false &&
				(isUnBlocked(i + 1, j + 1) == true && isUnBlocked(i, j + 1) == true && isUnBlocked(i + 1, j) == true))
			{
				gNew = cellDetails[i][j].g + 1.414;
				hNew = calculateHValue(i + 1, j + 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i + 1][j + 1].f == FLT_MAX ||
					cellDetails[i + 1][j + 1].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i + 1, j + 1)));

					// Update the details of this cell 
					cellDetails[i + 1][j + 1].f = fNew;
					cellDetails[i + 1][j + 1].g = gNew;
					cellDetails[i + 1][j + 1].h = hNew;
					cellDetails[i + 1][j + 1].parent_i = i;
					cellDetails[i + 1][j + 1].parent_j = j;
				}
			}
		}

		//----------- 8th Successor (South-West) ------------ 

		// Only process this cell if this is a valid one 
		if (isValid(i + 1, j - 1) == true)
		{
			// If the destination cell is the same as the 
			// current successor 
			if (isDestination(i + 1, j - 1, dest) == true && isUnBlocked(i, j - 1) == true && isUnBlocked(i + 1, j) == true)
			{
				// Set the Parent of the destination cell 
				cellDetails[i + 1][j - 1].parent_i = i;
				cellDetails[i + 1][j - 1].parent_j = j;
				//printf("The destination cell is found\n");
				//tracePath(cellDetails, dest);
				foundDest = true;
				return cellDetails;
			}

			// If the successor is already on the closed 
			// list or if it is blocked, then ignore it. 
			// Else do the following 
			else if (closedList[i + 1][j - 1] == false &&
				(isUnBlocked(i + 1, j - 1) == true && isUnBlocked(i, j - 1) == true && isUnBlocked(i + 1, j) == true))
			{
				gNew = cellDetails[i][j].g + 1.414;
				hNew = calculateHValue(i + 1, j - 1, dest);
				fNew = gNew + hNew;


				if (cellDetails[i + 1][j - 1].f == FLT_MAX ||
					cellDetails[i + 1][j - 1].f > fNew)
				{
					openList.insert(make_pair(fNew,
						make_pair(i + 1, j - 1)));

					// Update the details of this cell 
					cellDetails[i + 1][j - 1].f = fNew;
					cellDetails[i + 1][j - 1].g = gNew;
					cellDetails[i + 1][j - 1].h = hNew;
					cellDetails[i + 1][j - 1].parent_i = i;
					cellDetails[i + 1][j - 1].parent_j = j;
				}
			}
		}
	}

	// When the destination cell is not found and the open 
	// list is empty, then we conclude that we failed to 
	// reach the destiantion cell. This may happen when the 
	// there is no way to destination cell (due to blockages) 
	if (foundDest == false)
		printf("Failed to find the Destination Cell\n");

	return NULL;
}
