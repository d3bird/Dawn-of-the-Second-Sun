#include "beast_manager.h"

beast_manager::beast_manager(){
	amount = 0;
    max_amount = 10;
    beast_shader = NULL;
}

beast_manager::~beast_manager(){

}


void beast_manager::draw() {
    updateBuffer();

    beast_shader->use();
    beast_shader->setMat4("projection", projection);
    beast_shader->setMat4("view", view);
    beast_shader->setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, beast->textures_loaded[0].id);
    for (unsigned int i = 0; i < beast->meshes.size(); i++)
    {
        glBindVertexArray(beast->meshes[i].VAO);
        glDrawElementsInstanced(GL_TRIANGLES, beast->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount);
        glBindVertexArray(0);
    }
}

float beast_manager::diff_btwn_pnt(float start, float end) {
    float output = 0;
    if (start < end) {
        output = end - start;
    }
    else {
        output = start - end;
    }
    return output;
}

//determins whther to move the loc in a positive or negative direction
bool beast_manager::determin_direction(float start, float end) {
    if (start < end) {
        return true;//possitive
    }
    return false;//negative
}

void beast_manager::update() {
    if (condional_jobs.size() > 0) {
        check_conditional_jobs();
    }

    float speed = (*deltatime) * 30;

    glm::mat4 trans = glm::mat4(1.0f);
    glm::vec3* nav_point;
    glm::vec3* current_loc;

    //procces the all_creatures beasts
    for (unsigned int i = 0; i < all_creatures.size(); i++) {
        trans = glm::mat4(1.0f);
        nav_point = all_creatures[i]->get_next_nav_point();


        if (nav_point != NULL) {

            current_loc = all_creatures[i]->get_loc();
            trans = glm::scale(trans, glm::vec3(0.25, 0.25, 0.25));

            bool reached_x = false;
            bool reached_z = false;
            float temp;//remaining distance
            //move x
            temp = diff_btwn_pnt(current_loc->x, nav_point->x);

            if (speed <= temp) {
                // std::cout << current_loc->x << " " << current_loc->y << " " << current_loc->z << " || " << speed << " <> " << temp << " && " << nav_point->x << std::endl;
                if (determin_direction(current_loc->x, nav_point->x)) {
                    current_loc->x += speed;
                }
                else {
                    current_loc->x -= speed;
                }
            }
            else {
                //std::cout << "can not move anymore on x axis" << std::endl;
                current_loc->x = nav_point->x;
                reached_x = true;
            }

            //move z
            temp = diff_btwn_pnt(current_loc->z, nav_point->z);

            if (speed <= temp) {
                // std::cout << current_loc->x << " " << current_loc->y << " " << current_loc->z << " || " << speed << " <> " << temp << " && " << nav_point->x << std::endl;
                if (determin_direction(current_loc->z, nav_point->z)) {
                    current_loc->z += speed;
                }
                else {
                    current_loc->z -= speed;
                }
            }
            else {
                //std::cout << "can not move anymore on x axis" << std::endl;
                current_loc->z = nav_point->z;
                reached_z = true;
            }

            trans = glm::translate(trans, glm::vec3(current_loc->x, current_loc->y, current_loc->z));
            beast_matrices[all_creatures[i]->get_buffer_loc()] = trans;
             //update held items
            if (all_creatures[i]->is_holding_item()) {
                objects->update_item_matrix(all_creatures[i]->generate_item_update());
            }

            if (reached_z && reached_x) {
                //actions can only be preformed/ worked on if the unit is still moving to the point
                //so we do not pop the last one untill the action is preformed
                if (all_creatures[i]->get_travel_que_size() != 1) {
                    all_creatures[i]->pop_nav_point();
                }
                if (all_creatures[i]->get_travel_que_size()-1 == 0) {
                    // std::cout << "out of travel points" << std::endl;
                    work_order* current_job = all_creatures[i]->get_current_work_order();

                    bool can_preform = false;
                    if (map->action_requires_time(current_job) && current_job->time_spent< current_job->time_length) {
                        //map->print_currently_on(current_job);
                        current_job->time_spent += (*deltatime);
                        //std::cout << "time spent " << current_job->time_spent << " out of " << current_job->time_length << std::endl;
                    }
                    else {
                        can_preform = true;
                        all_creatures[i]->pop_nav_point();
                    }

                    if (can_preform) {
                        all_creatures[i]->swap_dest_loc();
                        preform_action(current_job, all_creatures[i]);

                        current_job->currently_on++;
                        if (current_job->currently_on >= current_job->action_numbers) {// no more actions for this job
                          //  std::cout << "jobs done" << std::endl;
                            map->delete_work_order(current_job);
                            all_creatures[i]->set_current_work_order(NULL);
                            if (all_creatures[i]->is_real_job()) {
                                need_jobs.push_back(all_creatures[i]);
                                remove_from_has_jobs(all_creatures[i]);
                            }
                            //  std::cout << "have_jobs  = " << have_jobs.size() << std::endl;
                            //  std::cout << "need_jobs  = " << need_jobs.size() << std::endl;
                        }
                        else {// move to the next destination

                            int x1 = all_creatures[i]->get_loc_map_x();
                            int z1 = all_creatures[i]->get_loc_map_z();
                            int x2 = current_job->destination[1].x;
                            int z2 = current_job->destination[1].z;
                            all_creatures[i]->set_loc_map_x_d(x2);
                            all_creatures[i]->set_loc_map_z_d(z2);
                            //  std::cout << "x1 = " << x1 << " z1 = " << z1 << " x2 = " << x2 << " z2 = " << z2 << std::endl;

                            std::vector<glm::vec3*> nav_points = map->find_path(z1, x1, z2, x2, 3);

                            if (nav_points.size() > 0) {
                                for (size_t q = 0; q < nav_points.size(); q++)
                                {
                                    glm::vec3* temp2 = nav_points[q];
                                    temp2->x *= 4;// compensate for the cubes scale
                                    temp2->z *= 4;
                                    all_creatures[i]->add_nav_point(nav_points[q]);
                                }
                            }

                        }
                    }
                }
            }
        }
        else {//if they do not have a job/point to move to 
            if (jobs_backlog.size() > 0) {
                assign_backlog_task(jobs_backlog.front());
                jobs_backlog.pop();
            }
            else {
               // std::cout << "creature needs to relax" << std::endl;
                work_order* relax_job = map->generate_work_order(RELAX,
                    all_creatures[i]->get_loc_map_x(), all_creatures[i]->get_loc_map_y(), all_creatures[i]->get_loc_map_z());
                create_tasks_for_creature(relax_job, all_creatures[i]);
            }
        }

    }

}

void beast_manager::init() {
    std::cout << "creating the beast_manger class" << std::endl;
    
    if (Time != NULL) {
        deltatime = Time->get_time_change();
    }
    else {
        std::cout << "there was a problem getting time in the sky" << std::endl;
        while (true);
    }

    if (beast_shader == NULL) {
        beast_shader = new Shader("shaders/asteroids.vs", "shaders/asteroids.fs");
    }
    else {
        std::cout << "using a premade shader for the beast_master object" << std::endl;
    }
    //beast = new Model("resources/objects/cube/cube.obj");
    beast = new Model("resources/objects/planet/planet.obj");

    beast_matrices = new glm::mat4[max_amount];

    float x = 0;
    float y = 3;
    float z = 0;

   /* std::vector<glm::vec3*> nav_points = map->find_path(8, 0, 0, 0, 3);

   for (unsigned int i = 0; i < amount; i++){
        creature* temp = new creature();
        temp->hold_item(objects->get_item_info());
        temp->set_scale(4);
        x = nav_points[0]->x * 4;
        z = nav_points[0]->z * 4;
        glm::vec3* start_loc = new glm::vec3(x, y, z);
        temp->set_loc(start_loc);
        temp->set_buffer_loc(i);
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::scale(trans, glm::vec3(0.25, 0.25, 0.25));
        trans = glm::translate(trans, glm::vec3(x, y, z));
        beast_matrices[i] = trans;

        for (size_t i = 0; i < nav_points.size(); i++)
        {
            glm::vec3* temp2 = nav_points[i];
            temp2->x *= 4;// compensate for the cubes scale
            temp2->z *= 4;
            temp->add_nav_point(nav_points[i]);
        }

        nav_points = map->find_path(0, 0, 8, 0, 3);

        for (size_t i = 0; i < nav_points.size(); i++)
        {
            glm::vec3* temp2 = nav_points[i];
            temp2->x *= 4;// compensate for the cubes scale
            temp2->z *= 4;
            temp->add_nav_point(nav_points[i]);
        }

        nav_points = map->find_path(9, 0, 4, 6, 3);

        for (size_t i = 0; i < nav_points.size(); i++)
        {
            glm::vec3* temp2 = nav_points[i];
            temp2->x *= 4;// compensate for the cubes scale
            temp2->z *= 4;
            temp->add_nav_point(nav_points[i]);
        }

       // all_creatures.push_back(temp);
        
        x += 4;
    }*/

    need_jobs = all_creatures;

   // std::cout << "all_creatures list = " << all_creatures.size() << std::endl;
   // std::cout << "need_jobs list = " << need_jobs.size() << std::endl;

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, max_amount * sizeof(glm::mat4), &beast_matrices[0], GL_STATIC_DRAW);

    for (unsigned int i = 0; i < beast->meshes.size(); i++)
    {
        unsigned int VAO = beast->meshes[i].VAO;
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

std::cout << "finished creating the beast_manger class" << std::endl;
}

void beast_manager::spawn_creature(zone* spawn_zone) {

    if (amount < max_amount && spawn_zone != NULL) {

        zone_loc* spawn_loc = spawn_zone->get_spawn_loc();

        // std::cout << "spawning creature" << std::endl;

        if (spawn_zone == NULL) {
            std::cout << "failed to spawn a creature" << std::endl;
            return;
        }

        //the map 
        int x_m = spawn_loc->x;
        int y_m = spawn_loc->y + 7;
        int z_m = spawn_loc->z;

        float x = x_m;
        float y = y_m;
        float z = z_m;
        glm::vec3* start_loc_map = new glm::vec3(x, y - 7, z);
        //factor in the cube offset
        x *= 2.0;

        z *= 2.0;
        //factor in the scale offset
        x *= 4;

        z *= 4;

        glm::vec3* start_loc = new glm::vec3(x, y, z);

        //std::cout << "x_m = " << x_m<< " y_m = " << y_m << " z_m = " << z_m << std::endl;
        //std::cout << "x = " << x / 4 << " y = " << y << " z = " << z / 4 << std::endl;

        creature* temp = new creature();
        //temp->hold_item(objects->get_item_info());
        temp->set_scale(4);


        temp->set_loc(start_loc);
        temp->set_loc_map(start_loc_map);

        temp->set_buffer_loc(amount);
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::scale(trans, glm::vec3(0.25, 0.25, 0.25));
        trans = glm::translate(trans, glm::vec3(x, y, z));
        beast_matrices[amount] = trans;
        amount++;
        all_creatures.push_back(temp);
        temp->set_loc_map_int(spawn_loc->x, spawn_loc->y, spawn_loc->z);
        need_jobs.push_back(temp);
    }
    else {
        std::cout << "can not spawn another creature, buffer filled" << std::endl;
    }
}

void beast_manager::assign_backlog_task(work_order* Job) {
    creature* Creature = need_jobs[need_jobs.size() - 1];
    //if(Creature->)
    //Creature->map_loc_check();
    Creature->set_has_job_buffer_loc(have_jobs.size());
    have_jobs.push_back(Creature);
    need_jobs.pop_back();
    //std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << std::endl;
    int x1 = Creature->get_loc_map_x();
    int z1 = Creature->get_loc_map_z();
    int x2 = Job->destination[0].x;
    int z2 = Job->destination[0].z;
    Creature->set_loc_map_x_d(x2);
    Creature->set_loc_map_z_d(z2);
    // std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << " going to "
      //   << x2 << " " << z2 << std::endl;
    std::vector<glm::vec3*> nav_points = map->find_path(z1, x1, z2, x2, 3);

    if (nav_points.size() > 0) {
        for (size_t i = 0; i < nav_points.size(); i++)
        {
            glm::vec3* temp2 = nav_points[i];
            temp2->x *= 4;// compensate for the cubes scale
            temp2->z *= 4;
            Creature->add_nav_point(nav_points[i]);
        }
    }
    Creature->set_current_work_order(Job);


    // std::cout << "need_jobs = " << need_jobs.size() << std::endl;
    // std::cout << "have_jobs = " << have_jobs.size() << std::endl;
}

void beast_manager::create_tasks(work_order* Job) {
    //std::cout << "creating task" << std::endl;
    if (need_jobs.size() > 0) {
        creature* Creature = need_jobs[need_jobs.size() - 1];
        if (Creature->get_current_job() == NULL || Creature->get_current_job() == NULL) {
            std::cout << "creature alread has a job/task" << std::endl;
            //while (true);
        }
        Creature->set_real_job(true);
        Creature->set_has_job_buffer_loc(have_jobs.size());
        have_jobs.push_back(Creature);
        need_jobs.pop_back();
       // std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << std::endl;
        int x1 = Creature->get_loc_map_x();
        int z1 = Creature->get_loc_map_z();
        int x2 =  Job->destination[0].x;
        int z2 =  Job->destination[0].z;
        Creature->set_loc_map_x_d(x2);
        Creature->set_loc_map_z_d(z2);
        //std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << " going to "
          //  << x2 << " " << z2 << std::endl;
        std::vector<glm::vec3*> nav_points = map->find_path(z1, x1, z2, x2, 3);
       // std::cout << "adahukjk" << std::endl;
        if (nav_points.size() > 0) {
          //  std::cout << "adding points to creature" << std::endl;
            for (size_t i = 0; i < nav_points.size(); i++)
            {
                glm::vec3* temp2 = nav_points[i];
                temp2->x *= 4;// compensate for the cubes scale
                temp2->z *= 4;
                Creature->add_nav_point(nav_points[i]);
            }
            //Creature->set_current_work_order(Job);
        }
        else {
            glm::vec3* temp2 = new glm::vec3(x1 * 8, 3, z1 * 8);
            Creature->add_nav_point(temp2);
        }
       Creature->set_current_work_order(Job);


       // std::cout << "need_jobs = " << need_jobs.size() << std::endl;
        //std::cout << "have_jobs = " << have_jobs.size() << std::endl;
    }
    else {
        jobs_backlog.push(Job);
       // std::cout << "no open workers, adding to the backlog" << std::endl;
    }
   // std::cout << "finished creating task" << std::endl;
}

//for creating recreation tasks like wandering/relaxing
void beast_manager::create_tasks_for_creature(work_order* Job, creature* Creature) {
    //std::cout << "creating task for a creature" << std::endl;
        if (Creature->get_current_job() == NULL || Creature->get_current_job() == NULL) {
      //      std::cout << "creature alread has a job/task" << std::endl;
            //while (true);
        }
        Creature->set_real_job(false);
        // std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << std::endl;
        int x1 = Creature->get_loc_map_x();
        int z1 = Creature->get_loc_map_z();
        int x2 = Job->destination[0].x;
        int z2 = Job->destination[0].z;
        Creature->set_loc_map_x_d(x2);
        Creature->set_loc_map_z_d(z2);
        //std::cout << Creature->get_loc_map_x() << " " << Creature->get_loc_map_z() << " going to "
          //  << x2 << " " << z2 << std::endl;
        std::vector<glm::vec3*> nav_points = map->find_path(z1, x1, z2, x2, 3);
        // std::cout << "adahukjk" << std::endl;
        if (nav_points.size() > 0) {
            //  std::cout << "adding points to creature" << std::endl;
            for (size_t i = 0; i < nav_points.size(); i++)
            {
                glm::vec3* temp2 = nav_points[i];
                temp2->x *= 4;// compensate for the cubes scale
                temp2->z *= 4;
                Creature->add_nav_point(nav_points[i]);
            }
            //Creature->set_current_work_order(Job);
        }
        else {
            glm::vec3* temp2 = new glm::vec3(x1 * 8, 3, z1 * 8);
            Creature->add_nav_point(temp2);
        }
        Creature->set_current_work_order(Job);

        // std::cout << "need_jobs = " << need_jobs.size() << std::endl;
        //std::cout << "have_jobs = " << have_jobs.size() << std::endl;

 //    std::cout << "finished creating task" << std::endl;
}

void beast_manager::preform_action(work_order* Job, creature* npc) {
   // std::cout << "preforming an action" << std::endl;
    switch (Job->action_rq[Job->currently_on]) {
    case PICK_UP:
      //  std::cout << "picking up item" << std::endl;
        if (Job->object->zone_location != NULL) {//if the object is being picked up from a zone

        }
        else {
           
        }
        map->remove_item_from_map(Job->object);
        Job->object->zone_location = Job->zone_location;//pass off the destination data to the object
        npc->hold_item(Job->object);
        break;
    case DROP:
        //std::cout << "dropping item" << std::endl;
        Job->object->x_m = npc->get_loc_map_x_d();
        //  Job->object->y_m = npc->get_loc_map_y_d();
        Job->object->z_m = npc->get_loc_map_z_d();
        npc->drop_item();
        map->add_item_to_map(Job->object);
        //map->print_map_items();
        break;
    case SAC_OBJ:
       // std::cout << "sacrificing item" << std::endl;
        Job->object->x_m = npc->get_loc_map_x_d();
        // Job->object->y_m = npc->get_loc_map_y_d();
        Job->object->z_m = npc->get_loc_map_z_d();
        map->add_item_to_alter(Job->object);
        if (!map->start_sac()) {//needs to create a latter job to be compleated
           // std::cout << "need to wait to start sacrifice, creating a conditional job" << std::endl;
            work_order_C* temp = new work_order_C;
            temp->cond = START_ALTER;
            temp->wo = map->generate_work_order(START_SACRIFICE,
                Job->object->x_m, Job->object->y_m, Job->object->z_m);
            condional_jobs.push_back(temp);
        }
        npc->drop_item();
        //map->print_map_items();
        break;
    case START_SAC:
        //std::cout << "starting sacrifice" << std::endl;
        map->start_sac();
        break;
    case TILL:
       // std::cout << "tilling soil" << std::endl;
        Job->farm_t->tilled = true;
        Job->farm_t->work_order_given = false;
        map->plant(Job->farm_t);
        break;
    case TEND:
       // std::cout << "tending plant" << std::endl;
        Job->farm_t->needs_tendning = false;
        Job->farm_t->work_order_given = false;
        Job->farm_t->halted_growth = false;
        break;
    case HARVEST:
       // std::cout << "harvesting plant" << std::endl;
        map->harvest_farm_tile(Job->farm_t);
        Job->farm_t->needs_tendning = true;
        Job->farm_t->work_order_given = false;
        Job->farm_t->needs_harvest = false;
        Job->farm_t->halted_growth = false;
        Job->farm_t->grow_time = 0;
      //  std::cout << "harvesting plant at " << Job->farm_t->loc->x << " " << Job->farm_t->loc->z << std::endl;

        break;
    default:
       // std::cout << "nothing needs to be done here" << std::endl;
        break;
    }
}

void beast_manager::check_conditional_jobs() {
    for (int i = 0; i < condional_jobs.size(); i++) {
        switch (condional_jobs[i]->cond)
        {
        case START_ALTER:
            if (map->is_alter_ready() && !map->is_alter_about_to_start()) {
                create_tasks(condional_jobs[i]->wo);
                condional_jobs[i] = condional_jobs[condional_jobs.size() - 1];
                condional_jobs.pop_back();
                i--;
                map->toggle_alter_about_to_start();
            }
            break;
        }
    }
}

void beast_manager::remove_from_has_jobs(creature* npc) {
    unsigned int buffer_loc = npc->get_has_job_buffer_loc();
    have_jobs[buffer_loc] = have_jobs[have_jobs.size() - 1];
    have_jobs[buffer_loc]->set_has_job_buffer_loc(buffer_loc);
    have_jobs.pop_back();
}