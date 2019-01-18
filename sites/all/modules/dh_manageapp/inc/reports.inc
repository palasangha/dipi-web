<?php


function dh_manage_at_schedule()
{
	global $user;
	$centre_id = arg(1);
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	
	$course = db_query("select c_id from dh_course where c_center=$centre_id")->fetchAssoc();
	

	$q = "Select ct_id, c_id, c_start, c_end, c_name, CONCAT(t_f_name, ' ', t_l_name) as 'teacher', ct_type as 'Type', t_gender from dh_course left join dh_course_teacher on ct_course=c_id left join dh_teacher on ct_teacher=t_id where c_start >= ((CURDATE() - INTERVAL 1 YEAR)) and (c_deleted = 0) and c_center=$centre_id order by c_start asc, ct_type desc";

	$result = db_query($q);
	$data = array();

	while($r = $result->fetchAssoc())
	{	
		$data[$r['c_id'].'||||'.$r['c_name'].'||||'.$r['c_start']][$r['t_gender']][] = array('type' => $r['Type'], 'teacher' => $r['teacher']) ;
		
	}
	$old_rows = $upcoming_rows = array();
	$gender = array('M', 'F');
	foreach($data as $course => $course_data)
	{
		unset($row);	
		$t = explode("||||", $course);
		$row[] = $t[1];

		foreach($gender as $g)
		{			
			$temp = "";
			if (isset($course_data[$g]))
			{
				foreach($course_data[$g] as $gender_data )
				{
					$temp .= $gender_data['teacher'].' - '.$gender_data['type'].'<br>' ;
				}
			}
			$row[] = $temp;
		}

		$row[] = l("View", "assign-teacher/$centre_id/".$t[0]);
		if ( $t[2]  <= date('Y-m-d') ) 

			$old_rows[] = $row;
		else
			$upcoming_rows[] = $row;
			
	}
	
	$header = array('Course Name', 'Male', 'Female', 'Action');
	$out = '<h3>'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';


	$completed = theme("table", array('header' => $header, 'rows' => $old_rows ));
	$fieldset_vars = array(
		'element' => array(
		'#title' => t('Completed courses'),
		'#collapsible' => true,
		'#collapsed' => true,
		'#value' => '',
		'#children' => $completed
		),
	);

	$out .= theme('fieldset', $fieldset_vars);


	$out .= '<h3>Upcoming courses</h3>';
	$out .= theme("table", array('header' => $header, 'rows' => $upcoming_rows ));


	return $out;
}






function dh_manageapp_report_form($form, &$form_state)
{
	global $user;
	$centre_id = '';
	if (arg(1) <> '')
	{
		$centre_id = arg(1);
	}
	if ( isset($form_state['storage']) )
	{

		$query="SELECT td_id,td_val1 from dh_type_detail where td_type='COURSE-TYPE' and td_active=1";

		$courses = db_query($query)->fetchAllKeyed();
		$courses[''] = 'Choose';
		$courses['All'] = 'All';

		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

		


		$breadcrumb = '';
		if ($centre_id <> '')
		{
			$breadcrumb .= l("Back to Dashboard", "centre/".$centre_id);	
		}
		if ($breadcrumb <> '')
		{
			$form['a'] = array('#markup' => '<h3>'.$breadcrumb.'</h3>');
		}

			$form['#attributes'] = array('class' => array('container-inline')); 
		
			$form['course'] = array('#title' => 'Course', '#type' => 'select', '#options' => $courses,'#required' => TRUE);
			$form['from_date'] = array(
				'#type' => 'textfield',
				'#title' => t('Start Date'),
				'#attributes' => array('placeholder' => t('Enter Start Date'),'id'=>'from_date','class' => array('from_date'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);

			$form['to_date'] = array(
				'#type' => 'textfield',
				'#title' => t('End Date'),
				'#attributes' => array('placeholder' => t('Enter End Date'),'id'=>'to_date','class' => array('to_date','datepicker'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);
		$InlineJS="(function ($) { $('.datepicker').datepicker({ dateFormat: 'dd-mm-yy' });
				$('.from_date').datepicker({ dateFormat: 'dd-mm-yy',maxDate: '0', 
	        		onSelect: function (selectedDate ) {
	            		var dt2 = $('.to_date');
	            	
	            	    dt2.datepicker( 'option', 'maxDate', 0);
	            		}
	            	});
	            $('.to_date').datepicker({ dateFormat: 'dd-mm-yy',maxDate: '0'}); 	
	            })(jQuery);";

			drupal_add_library('system', 'ui.datepicker');

			drupal_add_js($InlineJS, array('type' => 'inline', 'scope' => 'footer', 'weight' => 4));
			$form['sub'] = array('#type' => 'submit', '#value' =>'Go');
			$form['#submit'][]='dh_manageapp_report_display';

			$storage = $form_state['storage'];
			$course=$storage['course'];
 		 	$from_date=$storage['from_date'];
		 	$to_date=$storage['to_date'];
		 	$from_date=date('Y-m-d' , strtotime($from_date));
		 	$to_date=date('Y-m-d' , strtotime($to_date));
		 
		 	$Query="SELECT dh_course_stat.*,c_center,c_name,c_id 
		 			 FROM dh_course_stat,dh_course 
		 			where cs_course=c_id and 
		 				  c_center=$centre_id and 
		 				  c_course_type=$course and 
		 				  c_start between '$from_date' AND '$to_date' 
		 		 order by cs_course,cs_gender ASC";
		 	if($course == "All")
		 	{
		 		//remove the course type from where condition
		 		$Query="SELECT dh_course_stat.*,c_center,c_name,c_id 
		 			 FROM dh_course_stat,dh_course 
		 			where cs_course=c_id and 
		 				  c_center=$centre_id and 
		 				  c_start between '$from_date' AND '$to_date' 
		 		 order by cs_course,cs_gender ASC";
		 	}
		 
		 	   $result = db_query($Query);
		 	   
		 	   $OM=0;//old male
			   $OF=0;//old female
			   $NM=0;//new male
			   $NF=0;//new female
			   $Foreigner=0; //Foreigner
			   $Total = 0;
			   $TeacherArr=$rows2=$CourseArr=$CourseIDArr=array();
			   	while( $r = $result->fetchAssoc() )
				{
					$cs_course=$r['cs_course'];
					$c_id=$r['c_id'];
					if (!in_array($cs_course, $CourseArr))
					{
					    $CourseArr[] = $cs_course; 
					}
					if (!in_array($c_id, $CourseIDArr))
					{
					    $CourseIDArr[] = $c_id; 
					}
					$cs_status=$r['cs_status'];
					$cs_gender=$r['cs_gender'];
					$cs_old=$r['cs_old'];
					$cs_count=$r['cs_count'];
					$rows[$cs_course][0]=$r['c_name'];
					if($cs_gender == 'M' && $cs_status=='Attended' && $cs_old == '0')
					{
						$rows[$cs_course][1]=$cs_count; //new male
		
					}
					if($cs_gender == 'M' && $cs_status=='Attended' && $cs_old == '1')
					{
						$rows[$cs_course][2]=$cs_count; //old male
	
					}
					if($cs_gender == 'F' && $cs_status=='Attended' && $cs_old == '0')
					{
						$rows[$cs_course][3]=$cs_count; //new female
		
					}	
					if($cs_gender == 'F' && $cs_status=='Attended' && $cs_old == '1')
					{
						$rows[$cs_course][4]=$cs_count; //old female
		
					}
			  	}
		
			 
			  	
			foreach ($CourseIDArr as $key => $value) 
			{
				$CourseID=$value;
				
				$Query2="SELECT dh_course_teacher.ct_teacher,
								dh_teacher.t_f_name,
								dh_teacher.t_m_name,
								dh_teacher.t_l_name,
								dh_course_teacher.ct_type,
								dh_teacher.t_gender
						   FROM dh_course_teacher,dh_teacher,dh_course 
						  where dh_teacher.t_id=dh_course_teacher.ct_teacher and 
								dh_course_teacher.ct_course=dh_course.c_id and
		                        c_center=$centre_id and
		                        dh_course.c_id=$CourseID
			                and ct_status='Confirmed'";
					$result2 = db_query($Query2);

					$TeacherCount=$result2->rowCount();
					$rows[$CourseID][5]=$TeacherCount;
					while( $r2 = $result2->fetchAssoc() )
					{
						$Name=$r2['t_f_name'].' '.$r2['t_l_name'];
						
						$Gender=$r2['t_gender'];
						$Type=$r2['ct_type'];
						
						$rows2[] = array('0'=>$Name ,
					                    '1'=>$Gender,
					                    '2'=>$Type);
					} 

			}
			$rows[$cs_course][6]=0;
			$rows[$cs_course][7]=0;
			$rows[$cs_course][8]=0;

			foreach ($CourseArr as $key => $value) {
						ksort($rows[$value]);
			}
			ksort($rows);

			$header = array(
			    array('data' => t('Course Name')),
			    array('data' => t('New Male')),
			    array('data' => t('Old Male')),
			    array('data' => t('New Female')),
			    array('data' => t('Old Female')),
			    array('data' => t('Teacher')),
			    array('data' => t('Sevak Male')),
			    array('data' => t('Sevak Female')),
			    array('data' => t('Foreigner')) 
				);

			$header2 = array(
			    array('data' => t('Name')),
			    array('data' => t('Gender')),
			    array('data' => t('Type'))
			);


			 $total[] = array(array('data' => $NM , 'align' => 'center'),
			 				array('data' => $NM , 'align' => 'center'),
						 	array('data' =>$OM , 'align' => 'center'),
						 	array('data' => $NF, 'align' => 'center'),
						 	array('data' =>$OF , 'align' => 'center'),
						 	array('data' => $Query2, 'align' => 'center'),
						 	array('data' => 0, 'align' => 'center'),
						 	array('data' => 0, 'align' => 'center'),
						 	array('data' => $Foreigner, 'align' => 'center'));
			$form['myfieldset']['mytable'] = array(
			  '#theme' => 'table',
			  '#header' => $header,
			  '#rows' => $rows
			);
			$form['myfieldset']['mytable2'] = array(
			  '#theme' => 'table',
			  '#header' => $header2,
			  '#rows' => $rows2
			);
			return $form;
	}
	else
	{
		

		$query="SELECT td_id,td_val1 from dh_type_detail where td_type='COURSE-TYPE' and td_active=1";

		$courses = db_query($query)->fetchAllKeyed();
			$courses[''] = 'Choose';
		$courses['All'] = 'All';
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

		

	
		$breadcrumb = '';
		if ($centre_id <> '')
		{
			$breadcrumb .= l("Back to Dashboard", "centre/".$centre_id);	
		}
		if ($breadcrumb <> '')
		{
			$form['a'] = array('#markup' => '<h3>'.$breadcrumb.'</h3>');
		}

			$form['#attributes'] = array('class' => array('container-inline')); 
		
			$form['course'] = array('#title' => 'Course', '#type' => 'select', '#options' => $courses,'#required' => TRUE);
			$form['from_date'] = array(
				'#type' => 'textfield',
				'#title' => t('Start Date'),
				'#attributes' => array('placeholder' => t('Enter Start Date'),'id'=>'from_date','class' => array('from_date'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);

			$form['to_date'] = array(
				'#type' => 'textfield',
				'#title' => t('End Date'),
				'#attributes' => array('placeholder' => t('Enter End Date'),'id'=>'to_date','class' => array('to_date','datepicker'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);
		$InlineJS="(function ($) { $('.datepicker').datepicker({ dateFormat: 'dd-mm-yy' });
				$('.from_date').datepicker({ dateFormat: 'dd-mm-yy',maxDate: '-1d', 
	        		onSelect: function (selectedDate ) {
	            		var dt2 = $('.to_date');
	            	
	            	    dt2.datepicker( 'option', 'maxDate', 0);
	            		}
	            	});
	          
	            })(jQuery);";

			drupal_add_library('system', 'ui.datepicker');

			drupal_add_js($InlineJS, array('type' => 'inline', 'scope' => 'footer', 'weight' => 4));
			$form['sub'] = array('#type' => 'submit', '#value' =>'Go');
			$form['#submit'][]='dh_manageapp_report_display';
			return $form;
	}
}
function dh_manageapp_report_display($form, &$form_state)
{
	$form_state['storage'] = $form_state['input'];
	$form_state['rebuild'] = TRUE;
}

?>