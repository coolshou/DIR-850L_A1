<module>
	<service><?=$GETCFG_SVC?></service>
	<netatalk>		
		<?echo dump(3, "/netatalk");?>		
	</netatalk>
	<runtime>
		<device>
			<storage>
				<count><?echo query("/runtime/device/storage/count");?></count>
			</storage>
		</device>
		<share_path>
			<?echo dump(2, "/runtime/device/storage/disk/entry/mntp");?>
		</share_path>
	</runtime>		
</module>
